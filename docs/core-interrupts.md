# Core interrupt synchronization

`RVCSR` stores all implemented CSR state as explicit private members and arrays;
there is no CSR registration table or per-register function pointer. `mstatus`
uses `csr::MStatus`, so interrupt and memory checks read its fields directly.
`sip`, `sie` and `sstatus` are views; `fcsr` combines the independent `fflags` and
`frm` fields. Guest `time`/`timeh` reads check access and obtain the current time;
their raw storage and the placeholder cycle counters retain the existing behavior.
Reset restores the configured hart ID, implementation IDs and other defaults.
Guest SATP and STIMECMP writes return a `WriteEffect` that the Core uses to update
translation or the timer. Internal raw writes do not perform those effects.
Unknown addresses and RV32-only registers on RV64 remain invalid.
`RVCore` reads MIE/SIE/SUM directly
from `RVCSR::mstatus`, so trap entry and xRET cannot leave a separate status
cache stale. Only pending state is shared with IRQ producers; other CSR state
remains owned by the hart.

`RVCSR` owns the mutex protecting `mip` and its `sip` view, plus an atomic
`pendingHint` caching exactly `mip != 0`. Every pending CSR update refreshes
the hint while holding the mutex. The hint uses relaxed atomic access: it only
skips unnecessary scans; actual CSR reads acquire the mutex.

Device/controller threads call the existing `set_*_interrupt_*()` and
`clear_*_interrupt_*()` methods. These directly update pending bits under the
mutex, set the Core's wake notification, and notify the target hart after
unlocking. Multiple producers may publish concurrently. IRQ levels are visible
without waiting for the hart to execute; there is no pending-update snapshot.

Guest `mip`/`sip` CSR instructions hold the mutex across the entire read-modify-write,
including capture of the old value returned to the guest. Debug access uses the
same protected storage; raw pointers to pending CSRs are not available. The
existing guest writable-bit masks are preserved.

Interrupt scanning checks the hint before acquiring the mutex. When true, it
captures `mip` once and uses that snapshot for enable, delegation and priority
checks. Masked pending interrupts keep the hint true, so later enable changes
need no hint refresh.
Only the executing hart updates trap state.

`step()` scans before every instruction. `run()` scans every 0x1000 instructions,
and also right after any instruction that may make a pending
interrupt deliverable: guest writes to mstatus/sstatus, mie/sie, mip/sip or
mideleg (`WriteEffect::Interrupt`), and `mret`/`sret`. A pending interrupt is
therefore taken before the next instruction once it is enabled; xv6's scheduler
depends on this, as it disables interrupts again right after `intr_on()`.

`RVCore` owns its wake notification, stop flag and condition variable. The wake
predicate and condition-variable wait use the same mutex as the pending CSRs,
avoiding lost wakeups. `wait_for_interrupt(timeout)` consumes the notification
and returns `Notified`, `Stopped` or `Timeout`. The default timeout is infinite.
`wake()` only requests a recheck and does not publish IRQ levels; use the set/clear
methods for IRQ injection. Notifications do not imply WFI or trap eligibility.

By default, WFI checks `mip & mie` in the Core and uses the normal interrupt logic for
global enables, delegation and trap entry. Every producer (timers, the PLIC, the
framework) publishes mip and notifies the hart, so WFI waits without a timeout and
uses no CPU while asleep; harts never poll devices.
With the default configuration, `step()` can therefore block in WFI. With
`CONFIG_DIFFTEST`, WFI returns at once (see below).

`request_stop()` wakes the hart and leaves a persistent flag readable through
`stop_requested()`. `reset()` clears that flag and the IRQ/wait state along
with CPU registers, and requires the hart and all producers to be quiescent.
Before destruction, all producers and the hart must finish accessing the Core.
RVCPU stops and joins its owned hart threads before freeing them; callers
running a Core on an external thread must join that thread themselves.

Both `Core<word_t>` and `CPU<word_t>` expose a public virtual `request_stop()`.
The CPU implementation forwards the request to every core and returns without
joining. These methods can be called through the base interfaces while execution
is active, provided initialization, reset and destruction do not run concurrently.

```cpp
cpu->request_stop(); // Wake and request stop on all cores.
cpu->join();         // Wait for CPU-owned execution threads.
```

For a blocking `run()` invoked on an external host thread, join that thread too.

## Platform boundary

`RVCPU` and `RVCore` own no platform devices. The boundary has three parts:

- **Interrupt controller.** `device::InterruptController` (`include/device/intc.hpp`)
  is attached to the bus with `Bus::set_interrupt_controller()`. Devices are
  level-triggered: a device mapped with `add_mmio_map(id, ...)` drives IRQ source
  `id` with `MMIODev::set_irq_level()`, from any thread; source 0 has no line.
  The bus forwards each level to the controller's `set_irq()` and replays asserted
  sources to a newly attached one. The PLIC implements this interface: context 2i
  drives MEIP and 2i+1 SEIP of hart i whenever its output changes, which wakes a
  hart in WFI. Claim reads take the best pending source (priority above the
  threshold, ties to the lowest ID); a completion re-requests a source that is
  still asserted. Lock order is device, then PLIC, then the Core pending lock;
  each layer drives the next with its own lock held so levels arrive in order.
  Without a controller, the framework injects interrupts through the Core
  `set_*_interrupt()`/`clear_*_interrupt()` methods.
- **Device I/O.** `utils::IOLoop` (`include/utils/io-loop.hpp`) runs a standalone
  Asio `io_context` (fetched by CMake, header-only) on one background thread.
  Devices create Asio I/O objects on `context()` and `co_spawn` coroutines on it.
  A socket UART (`Uart16650::open_socket(loop, ip, port)`) receives with
  `co_await async_read_some()` on the loop and sends with blocking writes from the
  hart. `stop()` joins the thread; no handler runs afterwards, and waiting
  coroutines are destroyed with the loop without being resumed. Stop the loop
  before destroying the CPU, the platform or the devices, since a coroutine may
  reach the cores through the PLIC, and destroy Asio I/O objects before the loop.
  KDB owns one loop (`kdb::device::io_loop()`).
- **Asynchronous devices.** `VirtIOBlock` parses a request on the hart that
  writes QueueNotify, then serves it on its own `asio::thread_pool` with
  `preadv`/`pwritev` directly into guest memory, so the MMIO write returns at
  once. Requests may complete out of order: each completion writes the status,
  publishes the used ring (release store of `used->idx`) and raises the IRQ,
  all under the VirtIO lock. A device reset waits for in-flight requests.
  `MMIODev::quiesce()` waits until a device has no asynchronous work left;
  `kdb::device::stop()` stops the I/O loop and quiesces every device, and must run
  before the CPU, the platform or the devices are destroyed.
- **Time source.** `cpu::PlatformTimer` (`include/cpu/riscv/platform-timer.hpp`)
  is an interface in mtime ticks: `mtime()`, `schedule(deadline, cb)`, `cancel()`,
  plus optional `start()`/`stop()`/`reset()`. `RVCPU::set_platform_timer()` attaches
  it after `init()` and before `reset()`. The cores use it for the `time` CSR and
  Sstc `stimecmp`, and RVCPU starts and stops it around `run()`. Implementations
  must not run a callback synchronously inside `schedule()`, must not hold their own
  lock while running a callback, and must run no callback after `stop()` returns.
  Callbacks may fire early: the comparators recheck. Without a timer, `time` reads
  zero and `stimecmp` is only stored. `HostPlatformTimer` is the host steady-clock
  implementation: mtime pauses across `stop()`/`start()` and rewinds on `reset()`.
- **Built-in platform.** `cpu::RVPlatform` (`include/cpu/riscv/platform.hpp`)
  bundles a `HostPlatformTimer`, the ACLINT and the PLIC. `attach(cpu, bus)` must
  run after `cpu->init()`: it attaches the timer, maps the ACLINT and PLIC and
  makes the PLIC the bus interrupt controller. Call `reset()` alongside
  `cpu->reset()`. Destroy the CPU first, then the platform, then the bus. KDB does
  this through `isa::init_platform()`, `isa::reset_platform()` and
  `isa::deinit_platform()`.

```cpp
Bus bus;
RVPlatform platform; // Outlives cpu.
RVCPU cpu;
cpu.init(&bus, 0, coreCount);
platform.attach(&cpu, &bus);
cpu.reset(entry);
platform.reset();
```

`CONFIG_DIFFTEST` (Core Options; default on with `CONFIG_TARGET_SHARE`) builds
bare harts for differential testing. WFI returns at once, and KDB does not attach `RVPlatform`.
The framework injects interrupts and may attach its own `PlatformTimer`, for
example one that follows the DUT's mtime.

## Tests

`CONFIG_WFI_AS_NOP` defaults to disabled. Enable it under **Core Options** in
menuconfig, or pass `CONFIG_WFI_AS_NOP=y` to `make -f build.mk`, to execute WFI
as a NOP. This bypasses WFI-specific polling, waiting and interrupt scanning;
normal interrupt checks in `step()` and `run()` are unchanged. This option does
not add privilege/TW validation to the existing WFI implementation.

Enable the host tests in a RISC-V build; no guest cross-compiler is needed:

```sh
make -f build.mk CONFIG_ISA=riscv64 CMAKE_ARGS=-DKXEMU_BUILD_CORE_TESTS=ON
ctest --test-dir build --output-on-failure
```

The tests use Catch2 v3, which CMake downloads with `FetchContent` on the first
configure. Each `TEST_CASE` is registered as its own CTest test, so a subset can
be selected by name, or run directly by Catch2 tag:

```sh
ctest --test-dir build -R satp --output-on-failure
build/tests/host/core-interrupts/rvcore-interrupt-test "[concurrency]"
```

With `CONFIG_WFI_AS_NOP=y`, the same test target runs nonblocking WFI tests
instead of the blocking WFI suite. With `CONFIG_DIFFTEST=y`, only the difftest
suite (and the configuration-independent tests) is built. Use a separate build
directory for each mode.

Use `CONFIG_ISA=riscv32` with a separate build directory to test RV32. Tests
exercise actual Core stepping, including concurrent IRQ publication and guest
CSR updates, all six pending CSR instruction variants, hint maintenance, trap
and floating-point CSR views, mstatus changes across trap entry and MRET, repeated
assert/deassert updates, unrelated pending bits, bare wake notifications, hart
isolation, masked/delegated interrupts, WFI trap entry, stop/reset, device IRQs
waking WFI through the PLIC without spinning, bus IRQ forwarding, PLIC
claim/complete, the I/O loop, socket UART input, asynchronous virtio-blk
requests and CPU teardown.

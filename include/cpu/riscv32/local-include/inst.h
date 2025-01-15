void do_add();
void do_sub();
void do_and();
void do_or();
void do_xor();
void do_sll();
void do_srl();
void do_sra();
void do_slt();
void do_sltu();
    
void do_addi();
void do_andi();
void do_ori();
void do_xori();
void do_slli();
void do_srli();
void do_srai();
void do_slti();
void do_sltiu();

void do_lb();
void do_lbu();
void do_lh();
void do_lhu();
void do_lw();

void do_sb();
void do_sh();
void do_sw();

void do_beq();
void do_bge();
void do_bgeu();
void do_blt();
void do_bltu();
void do_bne();

void do_jal();
void do_jalr();

void do_lui();
void do_auipc();

void do_mul();
void do_mulh();
void do_mulhsu();
void do_mulhu();
void do_div();
void do_divu();
void do_rem();
void do_remu();
    
void do_invalid_inst();

// Compressed extension
void do_c_lwsp();
void do_c_swsp();
    
void do_c_lw();
void do_c_sw();

void do_c_j();
void do_c_jal();
void do_c_jr();
void do_c_jalr();

void do_c_beqz();
void do_c_bnez();

void do_c_li();
void do_c_lui();

void do_c_addi();
void do_c_addi16sp();
void do_c_addi4spn();

void do_c_slli();
void do_c_srli();
void do_c_srai();
void do_c_andi();

void do_c_mv();
void do_c_add();
void do_c_sub();
void do_c_xor();
void do_c_or();
void do_c_and();

// Privileged mode
void do_ecall();
void do_mret();
void do_sret();
void do_ebreak();

// Zicsr exntension
void do_csrrw();
void do_csrrs();
void do_csrrc();
void do_csrrwi();
void do_csrrsi();
void do_csrrci();
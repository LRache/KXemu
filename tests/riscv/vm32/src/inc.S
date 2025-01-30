.globl user_img_start
.globl user_img_end

.section .data
user_img_start:
    .incbin "./vm32/user-img/build/user.bin"
user_img_end:

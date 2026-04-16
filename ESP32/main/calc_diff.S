    .section .text
    .align  4
    .global calculate_total_diff
    .type   calculate_total_diff, @function

calculate_total_diff:
    # Wejście: a2 = buf1, a3 = buf2, a4 = len
    entry   a1, 32
    movi    a5, 0           # total = 0

	# Upewnij się, że len > 0
    beqz    a4, .L_done


.L_loop:
    blti    a4, 16, .L_done # Jeśli len < 16, koniec

    # Zamiast: ee.lq.128.ip q0, a2, 16
    .byte 0x0b, 0x20, 0x00, 0x72, 0x00, 0x00 # To jest surowy kod dla ee.lq.128.ip q0, a2, 16

    # Zamiast: ee.lq.128.ip q1, a3, 16
    .byte 0x1b, 0x30, 0x00, 0x72, 0x00, 0x00 # To jest surowy kod dla ee.lq.128.ip q1, a3, 16

    # Zamiast: ee.sad.8.u a6, q0, q1
    .byte 0x68, 0x01, 0x60, 0x72, 0x00, 0x00 # To jest surowy kod dla ee.sad.8.u a6, q0, q1

    add     a5, a5, a6      # total += a6
    isync                   # <--- WYMUŚ SYNCHRONIZACJĘ
    addi    a4, a4, -16     # len -= 16
    j       .L_loop

.L_done:
    mov     a2, a5          # return total
    retw

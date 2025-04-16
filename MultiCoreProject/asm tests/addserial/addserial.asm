#############################################################################################################
# Add Serial using one core
# Optimized Code with Loop Unrolling
#############################################################################################################
# Registers:
# $r2: Vector 1 base address
# $r3: Vector 2 base address
# $r4: Sum vector base address
# $r5: Loop counter (decrements by 4 per iteration)
# $r6-$r9: Temporary registers for Vector 1 elements
# $r10-$r13: Temporary registers for Vector 2 elements
# $r14-$r15: Temporary registers for sum
#############################################################################################################
# Load base addresses:
#############################################################################################################
add $r2, $zero, $zero, 0        # pc = 0, Base address of Vector 1 (0)
add $r3, $zero, $imm, 0x800     # pc = 1, Base address of Vector 2 is 4096 (0 + 2048), cont in next line
add $r3, $r3, $imm, 0x800       # pc = 2, Base address of Vector 2 is 4096 (2048 + 2048)
add $r4, $r3, $imm, 0x800       # pc = 3, Base address of Sum Vector is 8192 (4096 + 2048), cont in next line
add $r4, $r4, $imm, 0x800       # pc = 4, Base address of Sum Vector is 8192 (6144 + 2048)
add $r5, $zero, $imm, 1024      # pc = 5, Loop counter is 1024 (1024 iterations of 4 elements)
#############################################################################################################
# Unrolled Loop, 4 elements each iteration
#############################################################################################################
# Load 4 elements from Vector 1
lw $r6, $r2, $zero, 0       # pc = 6, Load V1[0]
lw $r7, $r2, $imm, 1        # pc = 7, Load V1[1]
lw $r8, $r2, $imm, 2        # pc = 8cd, Load V1[2]
lw $r9, $r2, $imm, 3        # pc = 9, Load V1[3]
# Load 4 elements from Vector 2
lw $r10, $r3, $zero, 0      # pc = 10, Load V2[0]
lw $r11, $r3, $imm, 1       # pc = 11, Load V2[1]
lw $r12, $r3, $imm, 2       # pc = 12, Load V2[2]
lw $r13, $r3, $imm, 3       # pc = 13, Load V2[3]
# Compute sums
add $r14, $r6, $r10, 0      # pc = 14, Sum V1[0] + V2[0]
add $r15, $r7, $r11, 0      # pc = 15, Sum V1[1] + V2[1]
sw $r14, $r4, $zero, 0      # pc = 16, Store sum in Sum[0]
sw $r15, $r4, $imm, 1       # pc = 17, Store sum in Sum[1]
add $r14, $r8, $r12, 0      # pc = 18, Sum V1[2] + V2[2]
add $r15, $r9, $r13, 0      # pc = 19, Sum V1[3] + V2[3]
sw $r14, $r4, $imm, 2       # pc = 20, Store sum in Sum[2]
sw $r15, $r4, $imm, 3       # pc = 21, Store sum in Sum[3]
# Update addresses
add $r2, $r2, $imm, 4       # pc = 22, Increment Vector 1 address by 4
add $r3, $r3, $imm, 4       # pc = 23, Increment Vector 2 address by 4
add $r4, $r4, $imm, 4       # pc = 24, Increment Sum Vector address by 4
sub $r5, $r5, $imm, 1       # pc = 25, Decrement loop counter by 4
bne $imm, $r5, $zero, 6     # pc = 26, Continue loop if counter != 0
add $zero, $zero, $zero, 0  # pc = 27, NOP after branch
#############################################################################################################
# Create conflict misses Part
#############################################################################################################
add $r6, $r5, $zero, 0      # pc = 28, $r6 = $r5 = 0
add $r5, $zero, $imm, 256   # pc = 29, $r5 = 256, Loop counter for 256 addresses
# Loop for creating conflict misses
lw $r4, $r6, $zero, 0          # pc = 30, Load word from memory (create conflict miss)
add $r6, $r6, $imm, 4          # pc = 31, Increment address by 1
sub $r5, $r5, $imm, 4          # pc = 32, Decrement loop counter
bne $imm, $r5, $zero, 30       # pc = 33, Continue loop if $r5 != 0
add $zero, $zero, $zero, 0     # pc = 34, NOP after branch
#############################################################################################################
# Halt
#############################################################################################################
halt $zero, $zero, $zero, 0     # pc = 35, Halt program

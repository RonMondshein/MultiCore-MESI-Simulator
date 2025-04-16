# r2 is current core number
# r3 is global counter for all cores
# r4 is local counter for current core
# r5 is mod4 result with global counter determine which core has the turn to excexute 
add $r4, $zero, $imm, 128		# PC:0, Initialize the local counter (r4) to 128
add $r2, $imm, $zero, 3		    # PC:1, Set core number to 3
lw $r3, $zero, $zero, 0		    # PC:2, Load the counter value from address 0 into register r3 (global counter value)
and $r5, $r3, $imm, 3			# PC:3, Perform a bitwise AND operation on r3 and r5 to get modulo 4 of the global counter (r3)
bne $imm, $r5, $r2, 2			# PC:4, If r5 (modulo 4 result) is not equal to the core number (r2), jump to polling (PC = 2)
add $zero, $zero, $zero, 0		# PC:5, NOP after branch
sub $r4, $r4, $imm, 1			# PC:6, Decrement the local counter (r4) by 1
add $r3, $r3, $imm, 1			# PC:7, Increment the global counter (r3) by 1 
sw $r3, $zero, $zero, 0		    # PC:8, Store the incremented global counter value (r3) back into memory address 0
bne $imm, $r4, $zero, 2		    # PC:9, If the local counter (r4) is not zero, continue looping (PC = 2)
add $zero, $zero, $zero, 0		# PC:10, NOP after branch
lw $r6, $zero, $imm, 256		# PC:11, r6 = MEM[256], force conflict miss for test 
halt $zero, $zero, $zero, 0	    # PC:12, HALT

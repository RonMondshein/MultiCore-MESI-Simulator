# r2 is current core number
# r3 is global counter for all cores
# r4 is local counter for current core
# r5 is mod4 result with global counter determine which core has the turn to excexute 
lw $r3, $zero, $zero, 0		    # PC:0, Load the counter value from address 0 into register r3 (global counter value)
add $r4, $zero, $imm, 128		# PC:1, Initialize the local counter (r4) to 128 
add $r2, $zero, $zero, 0		# PC:2, Set core number to 0
add $r3, $r3, $imm, 1			# PC:3, Increment the global counter (r3) by 1
sw $r3, $zero, $zero, 0		    # PC:4, Store the updated global counter value (r3) back into memory address 0
sub $r4, $r4, $imm, 1			# PC:5, Decrement the local counter (r4) by 1
lw $r3, $zero, $zero, 0		    # PC:6, Reload the updated global counter value into r3
and $r5, $r3, $imm, 3			# PC:7, Perform a bitwise AND operation on r3 and r5 to get modulo 4 of the global counter (r3)
bne $imm, $r5, $r2, 6			# PC:8, If r5 (modulo 4 result) is not equal to the core number (r2), jump to polling (PC = 6)
add $zero, $zero, $zero, 0		# PC:9, NOP after branch
sub $r4, $r4, $imm, 1			# PC:10, Decrement the local counter (r4) by 1 
add $r3, $r3, $imm, 1			# PC:11, Increment the global counter (r3) by 1 
sw $r3, $zero, $zero, 0		    # PC:12, Store the incremented global counter value (r3) back into memory address 0
bne $imm, $r4, $zero, 6		    # PC:13, If the local counter (r4) is not zero, continue looping (PC = 6)
add $zero, $zero, $zero, 0		# PC:14, NOP after branch
halt $zero, $zero, $zero, 0	    # PC:15, Halt
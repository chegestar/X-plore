
extern"C"{

void _call_via_r1(){ asm("stmfd sp!, {lr}\n mov lr, pc\n bx r1\n ldmfd sp!, {lr}"); }
void _call_via_r2(){ asm("stmfd sp!, {lr}\n mov lr, pc\n bx r2\n ldmfd sp!, {lr}"); }
void _call_via_r3(){ asm("stmfd sp!, {lr}\n mov lr, pc\n bx r3\n ldmfd sp!, {lr}"); }
void _call_via_r7(){ asm("stmfd sp!, {lr}\n mov lr, pc\n bx r7\n ldmfd sp!, {lr}"); }

}

//using spells parameter Duration from DATA.BIN instead of constant coding
void __declspec(naked) fix_StoneCurseDurationCalculation()
{ // 005233E6,
// 525412 ???????
	__asm
	{
		cmp		dword ptr [esp],0x05233E6+6
		jne		call2
call1:
		mov		edx, [ebp-0x0C]
		jmp		calc
call2:	//cmp		dword ptr [esp],0x0525412+6
		//mov		edx, [ebp-0x02FC]
		mov		edx,0x0F*0x10
calc:
		mov		edx, [edx+4]
		mov		edx, [edx+0x0C]
		mov		edx, [edx+0x0E*4]
		imul	edx
		ret
    }
}

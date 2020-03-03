%define GetMemberByte(register, base, member) mov register, BYTE[base + member]
%define SetMemberByte(register, base, member) mov BYTE[base + member], register

%define GetMemberWord(register, base, member) mov register, WORD[base + member]
%define SetMemberWord(register, base, member) mov WORD[base + member], register

%define GetMemberDWord(register, base, member) mov register, DWORD[base + member]
%define SetMemberDWord(register, base, member) mov DWORD[base + member], register
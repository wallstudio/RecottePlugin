.data
extern PA : qword
.code
JMPtoAPI proc
jmp qword ptr [PA]
JMPtoAPI endp
end
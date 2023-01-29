SECTION .data
message db "123+4567", 0h
debug db "error!", 0h
lenX:dd 1 ;4byte
lenY:dd 1 ;4byte

SECTION .bss
msgX:resb 255 ;存放X输入时的字符串
msgY:resb 255 ;存放Y输入时的字符串

Xarr: resd 255 ; 4byte each
Yarr: resd 255
num:resd 1; used in debug
SECTION .text
global  main
printChar:
    mov eax,4
    mov ebx,1
    int 80h
    ret
    
calXLen:
    dec eax
    mov ecx,eax
    sub eax,ebx
    mov [lenX],eax
    mov eax,ecx
    inc eax
    ret

switch:
    mov eax,[lenX]
    mov ebx,[lenY]
    mov [lenX],ebx
    mov [lenY],eax
    ret
judgeBigger:
    mov eax,[lenX]
    mov ebx,[lenY]
    sub eax,ebx
    cmp eax,0
    jle switch ;jump if lenX<lenY
    ret
    
main:
    mov ebp, esp; for correct debugging

    
input:
;prompt for X
   ; mov eax,4
    ;mov ebx,1
    ;mov ecx,messageX
    ;mov edx,17
    ;int 80h



    mov ebx, message        ; move the address of our message string into EBX
    mov eax, ebx        ; move the address in EBX into EAX as well (Both now point to the same segment in memory)

    
    mov edx,Xarr
nextchar:
    cmp     dword[eax], 0   ; compare the byte pointed to by EAX at this address against zero (Zero is an end of string delimiter)
    jz      calculateLen        ; jump (if the zero flagged has been set) to the point in the code labeled 'finished'
    cmp     dword[eax], '+'
    call    calXLen
    inc     eax             ; increment the address in EAX by one byte (if the zero flagged has NOT been set)
    jmp     nextchar        ; jump to the point in the code labeled 'nextchar'
 
calculateLen:
    sub     eax, ebx      
    
      
    
    
    
;calculate the length of Y
    ;mov [lenY],eax
    ;mov edx,[lenX]
    ;sub eax,edx
    ;mov [lenY],eax
    
    
    mov dword[num+1],0xa
    
    ;add eax,'0'
    add eax,ebx
    mov [num],eax
    mov ecx,num
    mov eax,4
    mov ebx,1
    mov edx,4
    int 80h
    
    
    
    
    
;debug
    mov dword[lenY+1],0xa ;\n,used in output
;输出刚刚输入的字符串
    mov eax,[lenX]
    add eax,'0'
    mov [num],eax
    ;mov dword[ecx],'1'
    mov ecx,num
    mov eax,4
    mov ebx,1
    mov edx,4
    int 80h
    

    mov eax,4
    mov ebx,1
    mov ecx,message
    mov edx,8
    int 80h
 


    mov     ebx, 0
    mov     eax, 1
    int     80h

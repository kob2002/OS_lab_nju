SECTION .data
messageX db "Please input x: ", 0h
messageY db "Please input x: ", 0h


SECTION .bss
msgX:resb 255 ;存放X输入时的字符串
msgY:resb 255 ;存放Y输入时的字符串
lenX:resd 1 ;4byte
lenY:resd 1 ;4byte
Xarr: resb 255
Yarr: resb 255
 
SECTION .text
global  _main
 
_main:
    mov ebp, esp; for correct debugging
input:
;prompt for X
    mov eax,4
    mov ebx,1
    mov ecx,messageX
    mov edx,17
    syscall
;read x
    mov eax,3
    mov ebx,2
    mov ecx,msgX
    mov edx,255
    int 80h


    mov ebx, msgX        ; move the address of our message string into EBX
    mov eax, ebx        ; move the address in EBX into EAX as well (Both now point to the same segment in memory)

    
    mov ecx,0 ;ecx用于遍历字符串
nextchar:
    cmp     byte [eax], 0   ; compare the byte pointed to by EAX at this address against zero (Zero is an end of string delimiter)
    jz      calculateLen        ; jump (if the zero flagged has been set) to the point in the code labeled 'finished'
    mov     [Xarr+ecx*2], eax
    sub     dword[Xarr+ecx*2],'0'
    inc     ecx
    inc     eax             ; increment the address in EAX by one byte (if the zero flagged has NOT been set)
    jmp     nextchar        ; jump to the point in the code labeled 'nextchar'
 
calculateLen:
    sub     eax, ebx        ; subtract the address in EBX from the address in EAX
                            ; remember both registers started pointing to the same address (see line 15)
                            ; but EAX has been incremented one byte for each character in the message string
                            ; when you subtract one memory address from another of the same type
                            ; the result is number of segments between them - in this case the number of bytes
    mov [lenX],eax
   
    
;输出刚刚输入的字符串
 

   
 


    mov     ebx, 0
    mov     eax, 1
    int     80h


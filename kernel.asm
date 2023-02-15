GLOBAL k_print, go, dispatch, lidtr, outportb, init_timer_dev
extern enqueue_priority, dequeue, running_process, pcb_queue

k_print:
    push ebp
    mov ebp, esp
    pushf
    push eax
    push ebx
    push ecx
    push esi
    push edi

    mov ebx, [ebp + 20]     ; pushing col to eax
    mov eax, [ebp + 16]     ; pushing row to ebx
    mov ecx, [ebp + 12]     ; pushing string length to ecx
    mov esi, [ebp + 8]      ; pushing string addr to esi

    imul eax, 80            ; row*80
    add eax, ebx            ; row*80+col
    imul eax, 2             ; (row*80+col) * 2
    add eax, 0xB8000        ; adding (ro*80+col)*2 with 0xB80000 to calculate offset
    mov edi, eax

    loop:
        cmp edi, 0xB8F9E    ; comparing current spot in video memory to last spot in video memory
        je done            ; if we are passed the last video memory address, go back to the top
        cmp ecx, 0          ; checking if string length is 0 to see if we are at the end of the string
        je done
        movsb               ; move byte at address DS:(E)SI to address ES:(E)DI.
        mov BYTE [edi], 140 ; moving a red on grey color byte into memory (140)
        inc edi
        dec ecx
        jmp loop

done:
    pop edi
    pop esi
    pop ecx
    pop ebx
    pop eax
    popf
    pop ebp
    ret

dispatch:
    pushad
    push gs
    push fs
    push es
    push ds

    mov eax, [running_process]
    mov [eax], esp
    push eax
    push pcb_queue
    call enqueue_priority
    add esp, 8
    push pcb_queue
    call dequeue
    mov [running_process], eax
    mov esp, [eax]

    pop gs
    pop fs
    pop es
    pop ds
    popad

    push eax

    mov al, 0x20
    out 0x20, al

    pop eax

    iret

go:
    pushad
    push ds
    push fs
    push es
    push gs
    push pcb_queue
    call dequeue
    add esp, 4
    mov [running_process], eax
    mov esp, [eax]

    pop gs
    pop fs
    pop es
    pop ds
    popad
    iret

lidtr:
    push ebp
    mov ebp, esp

    mov eax, [ebp+8]
    lidt [eax]
    pop ebp
    ret

outportb:
    push ebp
	mov ebp, esp
	push eax
	push edx

	mov eax, [ebp+12] 	
	mov edx, [ebp+8]	
	out dx, al

	pop edx
	pop eax
	pop ebp
	ret

init_timer_dev:
    push ebp

    mov ebp, esp

    pushfd
    pushad

    mov edx, [ebp+8]
    mov ax, 1193

    imul dx, ax

    mov al, 0b00110110
    out 0x43, al
    mov ax, dx

    out 0x40, al
    xchg ah, al
    out 0x40, al

    popad
    popfd
    pop ebp
    
    ret
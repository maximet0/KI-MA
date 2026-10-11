option casemap:none

.code

PUBLIC Kernel6x16_F32, Kernel1x64_F32

Kernel6x16_F32 PROC FRAME
	;	https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170
	;	Input: 
	;		RCX = const float** a
	;		RDX = const float* packedB
	;		R8 = float** c
	;		R9 = size_t K
	; Winodws ABI setzt voraus das die Register XMM6-XMM15 sowie R12-R15 gesichert werden müssen
	push r12
	.pushreg r12
	push r13
	.pushreg r13
	push r14
	.pushreg r14
	push r15
	.pushreg r15

	sub rsp, 160	; Sicherungsbereich für XMM6-XMM15 (10 Register * 16 Bytes = 160 Bytes) auf dem Stack reservieren
	.allocstack 160

	vmovdqu xmmword ptr [rsp + 0], xmm6
	.savexmm128 xmm6, 0
	vmovdqu xmmword ptr [rsp + 16], xmm7
	.savexmm128 xmm7, 16
	vmovdqu xmmword ptr [rsp + 32], xmm8
	.savexmm128 xmm8, 32
	vmovdqu xmmword ptr [rsp + 48], xmm9
	.savexmm128 xmm9, 48
	vmovdqu xmmword ptr [rsp + 64], xmm10
	.savexmm128 xmm10, 64
	vmovdqu xmmword ptr [rsp + 80], xmm11
	.savexmm128 xmm11, 80
	vmovdqu xmmword ptr [rsp + 96], xmm12
	.savexmm128 xmm12, 96
	vmovdqu xmmword ptr [rsp + 112], xmm13
	.savexmm128 xmm13, 112
	vmovdqu xmmword ptr [rsp + 128], xmm14
	.savexmm128 xmm14, 128
	vmovdqu xmmword ptr [rsp + 144], xmm15
	.savexmm128 xmm15, 144

	.endprolog

	; Akkumulatoren initialisieren
	vpxor ymm4, ymm4, ymm4		; 0,0
	vpxor ymm5, ymm5, ymm5		; 0,1
	vpxor ymm6, ymm6, ymm6		; 1,0
	vpxor ymm7, ymm7, ymm7		; 1,1
	vpxor ymm8, ymm8, ymm8		; 2,0
	vpxor ymm9, ymm9, ymm9		; 2,1
	vpxor ymm10, ymm10, ymm10	; 3,0
	vpxor ymm11, ymm11, ymm11	; 3,1
	vpxor ymm12, ymm12, ymm12	; 4,0
	vpxor ymm13, ymm13, ymm13	; 4,1
	vpxor ymm14, ymm14, ymm14	; 5,0
	vpxor ymm15, ymm15, ymm15	; 5,1

	mov r10, qword ptr [rcx]					; r10 = a
	mov r11, qword ptr [rcx + 8]				; r11 = a
	mov r12, qword ptr [rcx + 16]				; r12 = a
	mov r13, qword ptr [rcx + 24]				; r13 = a
	mov r14, qword ptr [rcx + 32]				; r14 = a
	mov r15, qword ptr [rcx + 40]				; r15 = a

	xor rax, rax				; K-Index auf 0 setzen
	test r9, r9				; Prüfen, ob K == 0
	jz storeResults			; Falls K == 0, springe zu storeResults
	kLoop:
		vmovups ymm2, ymmword ptr [rdx]			; Lade 8 Float-Werte von B in ymm2
		vmovups ymm3, ymmword ptr [rdx + 32]		; Lade die nächsten 8 Float-Werte von B in ymm3

		vbroadcastss ymm0, dword ptr [r10 + rax * 4]			; Lade a[0][k] und broadcast in ymm0
		vbroadcastss ymm1, dword ptr [r11 + rax * 4]		; Lade a[1][k] und broadcast in ymm1

		vfmadd231ps ymm4, ymm0, ymm2		; ymm4 += ymm0 * ymm2
		vfmadd231ps ymm5, ymm0, ymm3		; ymm5 += ymm0 * ymm3
		vfmadd231ps ymm6, ymm1, ymm2		; ymm6 += ymm1 * ymm2
		vfmadd231ps ymm7, ymm1, ymm3		; ymm7 += ymm1 * ymm3

		vbroadcastss ymm0, dword ptr [r12 + rax * 4]		; Lade a[2][k] und broadcast in ymm0
		vbroadcastss ymm1, dword ptr [r13 + rax * 4]	; Lade a[3][k] und broadcast in ymm1

		vfmadd231ps ymm8, ymm0, ymm2		; ymm8 += ymm0 * ymm2
		vfmadd231ps ymm9, ymm0, ymm3		; ymm9 += ymm0 * ymm3
		vfmadd231ps ymm10, ymm1, ymm2		; ymm10 += ymm1 * ymm2
		vfmadd231ps ymm11, ymm1, ymm3		; ymm11 += ymm1 * ymm3

		vbroadcastss ymm0, dword ptr [r14 + rax * 4]	; Lade a[4][k] und broadcast in ymm0
		vbroadcastss ymm1, dword ptr [r15 + rax * 4]	; Lade a[5][k] und broadcast in ymm1

		vfmadd231ps ymm12, ymm0, ymm2		; ymm12 += ymm0 * ymm2
		vfmadd231ps ymm13, ymm0, ymm3		; ymm13 += ymm0 * ymm3
		vfmadd231ps ymm14, ymm1, ymm2		; ymm14 += ymm1 * ymm2
		vfmadd231ps ymm15, ymm1, ymm3		; ymm15 += ymm1 * ymm3

		add rdx, 64				; Zeiger auf B um 16 Float-Werte (64 Bytes) erhöhen
		inc rax					; K-Index erhöhen
		cmp rax, r9				; K-Index mit K vergleichen
		jb kLoop				; Falls k < K, springe zu kLoop

	storeResults:
		; Speichern der Ergebnisse in C
		mov rax, qword ptr [r8]					; Lade die Adresse von C[0] in rax
		vmovups ymmword ptr [rax], ymm4			; Speichern von C[0][0..7]
		vmovups ymmword ptr [rax + 32], ymm5	; Speichern von C[0][8..15]
		mov rax, qword ptr [r8 + 8]
		vmovups ymmword ptr [rax], ymm6			; Speichern von C[1][0..7]
		vmovups ymmword ptr [rax + 32], ymm7	; Speichern von C[1][8..15]
		mov rax, qword ptr [r8 + 16]
		vmovups ymmword ptr [rax], ymm8			; Speichern von C[2][0..7]
		vmovups ymmword ptr [rax + 32], ymm9	; Speichern von C[2][8..15]
		mov rax, qword ptr [r8 + 24]
		vmovups ymmword ptr [rax], ymm10		; Speichern von C[3][0..7]
		vmovups ymmword ptr [rax + 32], ymm11	; Speichern von C[3][8..15]
		mov rax, qword ptr [r8 + 32]
		vmovups ymmword ptr [rax], ymm12		; Speichern von C[4][0..7]
		vmovups ymmword ptr [rax + 32], ymm13	; Speichern von C[4][8..15]
		mov rax, qword ptr [r8 + 40]
		vmovups ymmword ptr [rax], ymm14		; Speichern von C[5][0..7]
		vmovups ymmword ptr [rax + 32], ymm15	; Speichern von C[5][8..15]

	; Wiederherstellen der gesicherten Register
	vmovdqu xmm6, xmmword ptr [rsp + 0]
	vmovdqu xmm7, xmmword ptr [rsp + 16]
	vmovdqu xmm8, xmmword ptr [rsp + 32]
	vmovdqu xmm9, xmmword ptr [rsp + 48]
	vmovdqu xmm10, xmmword ptr [rsp + 64]
	vmovdqu xmm11, xmmword ptr [rsp + 80]
	vmovdqu xmm12, xmmword ptr [rsp + 96]
	vmovdqu xmm13, xmmword ptr [rsp + 112]
	vmovdqu xmm14, xmmword ptr [rsp + 128]
	vmovdqu xmm15, xmmword ptr [rsp + 144]

	vzeroupper

	add rsp, 160	; Den reservierten Stackbereich freigeben

	pop r15
	pop r14
	pop r13
	pop r12

	ret

Kernel6x16_F32 ENDP

Kernel1x64_F32 PROC FRAME
	;	https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170
	;	Input: 
	;		RCX = const float* a
	;		RDX = const float* packedB
	;		R8 = float* c
	;		R9 = size_t K

	; Winodws ABI setzt voraus das die Register XMM6-XMM15 sowie R12-R15 gesichert werden müssen
	push r12
	.pushreg r12
	push r13
	.pushreg r13

	sub rsp, 96	; Sicherungsbereich für XMM6-XMM11 (6 Register * 16 Bytes = 96 Bytes) auf dem Stack reservieren
	vmovdqu xmmword ptr [rsp + 0], xmm6
	.savexmm128 xmm6, 0
	vmovdqu xmmword ptr [rsp + 16], xmm7
	.savexmm128 xmm7, 16
	vmovdqu xmmword ptr [rsp + 32], xmm8
	.savexmm128 xmm8, 32
	vmovdqu xmmword ptr [rsp + 48], xmm9
	.savexmm128 xmm9, 48
	vmovdqu xmmword ptr [rsp + 64], xmm10
	.savexmm128 xmm10, 64
	vmovdqu xmmword ptr [rsp + 80], xmm11
	.savexmm128 xmm11, 80

	.endprolog

	vpxor ymm4, ymm4, ymm4		; Akkumulator initialisieren
	vpxor ymm5, ymm5, ymm5		; Akkumulator initialisieren
	vpxor ymm6, ymm6, ymm6		; Akkumulator initialisieren
	vpxor ymm7, ymm7, ymm7		; Akkumulator initialisieren
	vpxor ymm8, ymm8, ymm8		; Akkumulator initialisieren
	vpxor ymm9, ymm9, ymm9		; Akkumulator initialisieren
	vpxor ymm10, ymm10, ymm10	; Akkumulator initialisieren
	vpxor ymm11, ymm11, ymm11	; Akkumulator initialisieren

	mov rax, r9
	shl rax, 6		; rax = K * 64 (Anzahl der Bytes für K Iterationen von 16 Float-Werten)

	mov r10, rdx							; r10 = b
	lea r11, qword ptr [r10 + rax]			; r11 = b + k
	lea r12, qword ptr [r11 + rax]			; r12 = b + 2k
	lea r13, qword ptr [r12 + rax]			; r13 = b + 3k
	
	xor rax, rax				; K-Index auf 0 setzen
	test r9, r9				; Prüfen, ob K == 0
	jz storeResults			; Falls K == 0, springe zu storeResults
	
	kLoop:
		vbroadcastss ymm0, dword ptr [rcx + rax * 4]	; Lade a[k] und broadcast in ymm0

		vmovups ymm1, ymmword ptr [r10]			; Lade 8 Float-Werte von B in ymm2
		vmovups ymm2, ymmword ptr [r10 + 32]		; Lade die nächsten 8 Float-Werte von B in ymm3
		vfmadd231ps ymm4, ymm0, ymm1		; ymm4 += ymm0 * ymm1
		vfmadd231ps ymm5, ymm0, ymm2		; ymm5 += ymm0 * ymm1
		vmovups ymm1, ymmword ptr [r11]		; Lade die nächsten 8 Float-Werte von B in ymm4
		vmovups ymm2, ymmword ptr [r11 + 32]		; Lade die nächsten 8 Float-Werte von B in ymm5
		vfmadd231ps ymm6, ymm0, ymm1		; ymm6 += ymm0 * ymm1
		vfmadd231ps ymm7, ymm0, ymm2		; ymm7 += ymm0 * ymm1
		vmovups ymm1, ymmword ptr [r12]		; Lade die nächsten 8 Float-Werte von B in ymm6
		vmovups ymm2, ymmword ptr [r12 + 32]		; Lade die nächsten 8 Float-Werte von B in ymm7
		vfmadd231ps ymm8, ymm0, ymm1		; ymm8 += ymm0 * ymm1
		vfmadd231ps ymm9, ymm0, ymm2		; ymm9 += ymm0 * ymm1
		vmovups ymm1, ymmword ptr [r13]		; Lade die nächsten 8 Float-Werte von B in ymm8
		vmovups ymm2, ymmword ptr [r13 + 32]		; Lade die nächsten 8 Float-Werte von B in ymm9
		vfmadd231ps ymm10, ymm0, ymm1		; ymm10 += ymm0 * ymm1
		vfmadd231ps ymm11, ymm0, ymm2		; ymm11 += ymm0 * ymm1
		
		add r10, 64				
		add r11, 64
		add r12, 64
		add r13, 64

		inc rax					; K-Index erhöhen
		cmp rax, r9				; K-Index mit K vergleichen
		jb kLoop				; Falls k < K, springe zu kLoop


	storeResults:
		mov rax, r8							; Lade die Adresse von C[0] in rax
		vmovups ymmword ptr [rax], ymm4			; Speichern von C[0][0..7]
		vmovups ymmword ptr [rax + 32], ymm5	; Speichern von C[0][8..15]
		vmovups ymmword ptr [rax + 64], ymm6		; Speichern von C[0][16..23]
		vmovups ymmword ptr [rax + 96], ymm7		; Speichern von C[0][24..31]
		vmovups ymmword ptr [rax + 128], ymm8		; Speichern von C[0][32..39]
		vmovups ymmword ptr [rax + 160], ymm9		; Speichern von C[0][40..47]
		vmovups ymmword ptr [rax + 192], ymm10		; Speichern von C[0][48..55]
		vmovups ymmword ptr [rax + 224], ymm11		; Speichern von C[0][56..63]

	; Wiederherstellen der gesicherten Register
	vmovdqu xmm6, xmmword ptr [rsp + 0]
	vmovdqu xmm7, xmmword ptr [rsp + 16]
	vmovdqu xmm8, xmmword ptr [rsp + 32]
	vmovdqu xmm9, xmmword ptr [rsp + 48]
	vmovdqu xmm10, xmmword ptr [rsp + 64]
	vmovdqu xmm11, xmmword ptr [rsp + 80]

	vzeroupper

	add rsp, 96	; Den reservierten Stackbereich freigeben

	pop r13
	pop r12
	ret

Kernel1x64_F32 ENDP

END
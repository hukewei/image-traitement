; IMAGE.ASM
;
; MI01 - TP Assembleur 2 à 5
;
; Réalise le traitement d'une image 32 bits. 

.686
; Autoriser les instructions MMX 
.MMX
.MODEL FLAT, C

.DATA

.CODE

; **********************************************************************
; Sous-programme _process_image_asm 
; 
; Réalise le traitement d'une image 32 bits.
;
; Entrées sur la pile : Largeur de l'image (entier 32 bits)
;			Hauteur de l'image (entier 32 bits)
;			Pointeur sur l'image source (dépl. 32 bits)
;			Pointeur sur l'image tampon 1 (dépl. 32 bits)
;			Pointeur sur l'image tampon 2 (dépl. 32 bits)
;			Pointeur sur l'image finale (dépl. 32 bits)
; **********************************************************************

PUBLIC		process_image_mmx

process_image_mmx	PROC NEAR		; Point d'entrée du sous programme
		
		push    ebp
		mov     ebp, esp

		push    ebx
		push    esi
		push    edi
		
		mov     ecx, [ebp + 8]
		imul    ecx, [ebp + 12]

		mov     esi, [ebp + 16]
		mov     edi, [ebp + 20]

		;*****************************************************************
		;*****************************************************************
		PUSH EAX					; sauvegarder EAX
		PUSH EDX
		MOV EAX, 4D961Dh		
		MOVD MM0, EAX			
		PUNPCKLBW MM0, MM0		
		PSRLW MM0, 8			; On decale chaque word a droite
		
traitement:
		DEC ECX
		MOV EAX, [ESI + ECX*4]
		MOVD MM1,EAX
		PUNPCKLBW MM1, MM1
		PSRLW MM1, 8
		PMADDWD MM1,MM0
		MOVD EAX,MM1
		PSRLQ MM1,32
		MOVD MM2,EAX
		PADDD MM1,MM2
		PSRLQ MM1,8
		
		MOVD EAX, MM1
		MOV [EDI + ECX*4], EAX		; on enregistre le pixel dans l'image suivante
		CMP ECX, 0
		JNE traitement				; on repete jusqu'au dernier pixel

		
		MOV ESI, [EBP+20]		;esi = addresse de temp1
		MOV EDI, [EBP+24]
		MOV ECX, [EBP+8]
		IMUL ECX,4
		ADD EDI, ECX
		ADD EDI,4				;edi=[ebp+24]+4*largeur + 4
		MOV ECX, [EBP+12]		;ecx = hauteur
		SUB ECX,2
		SHL ECX,16				;partie haut de ecx = hauteur -2
		MOV EBP, [EBP+8]
		ADD CX,BP				
		SUB CX,2				;partie basse de ecx = largeur -2
		
		
CONTOUR :

		;calcule |Gx|
		mov  EBX,[esi+8]
		SUB  EBX,[esi]
		mov	 EAX,[esi + EBP*4]
		imul EAX,-2
		add  EBX,EAX
		mov  EAX,[esi + EBP*4 + 8]
		imul EAX,2
		add  EBX,EAX
		SUB  EBX,[esi+ebp*8]
		add  EBX,[ESI + EBP*8 + 8]
		cmp  EBX,0
		jg   gy
		neg  EBX;				;EBX =|Gx|

GY :	
		;calcule |Gy|
		MOV EDX,[ESI+4]
		IMUL EDX,2
		ADD EDX,[ESI]
		ADD EDX,[ESI+8]
		SUB EDX,[ESI+EBP*8]
		MOV EAX,[ESI+EBP*8+4]
		IMUL EAX,-2
		ADD EDX,EAX
		SUB EDX,[ESI+EBP*8+8]
		CMP EDX,0
		JG  calG
		NEG EDX

CALG :
		;calculer G=|Gx|+|Gy|
		ADD EDX,EBX
		SUB EDX,255
		NEG EDX
		MOV EAX,EDX
		CMP EAX,0
		JG  gris
		MOV EAX,0				;EAX = G

GRIS :
		MOV EDX, EAX
		SHL EDX, 8
		ADD EAX, EDX
		SHL EDX, 8
		ADD EAX, EDX

		MOV [EDI], EAX
		
		ADD ESI,4
		ADD EDI,4
		DEC CX
		JNZ contour
		
		ADD CX,BP
		SUB CX,2
		ADD ESI,8
		ADD EDI,8
		SUB ECX,10000h
		TEST ECX,0FFFF0000h
		JNZ contour

seuillage:
		MOV ESI,[ESP+44]
		MOV EDI,[ESP+48]

		;;calculer le nombre de pixels
		MOV ECX, [ESP + 28]
		IMUL ECX, [ESP + 32]
		

		;;generer le seuillage
		MOV EAX,000969696h
		MOVD MM1,EAX
		MOVD MM2,EAX
		PSLLQ MM1,32
		PADDD MM1,MM2	;MM1 seuillge maximum=150
		PXOR MM2,MM2	;MM2 seuillge =0
		
		
		;tester si ecx est paire ou impaire
		XOR EAX,EAX
		ADD EAX,1
		AND EAX,ECX
		CMP EAX,0
		JZ paire
		
		DEC ECX
		MOVD MM0, dword ptr[ESI+4 * ECX]
		PSUBUSB MM0,MM1				;si MM0 <=150, alors MM0=0, sinon MM0=MM0-150 (il est superieur a 0)
		PCMPGTB MM0,MM2				;Si MM0 est supperieur a 0, alors MM0=FFh, sinon MM0=00h
		MOVd dword ptr[EDI+4*ECX],MM0;
		
		
paire:		
		SHR ECX,1

boucle2:
		MOVQ MM0, qword ptr[ESI+8 * ECX]
		PSUBUSB MM0,MM1
		PCMPGTB MM0,MM2
	
		MOVQ qword ptr[EDI+8*ECX],MM0;
		dec ecx
		JNZ boucle2
			
fin:	
		POP EDX
		POP EAX	
		
		; Libérer l'unité MMX
		emms	
		pop     edi
		pop     esi
		pop     ebx

		pop     ebp

		ret			                ; Retour à la fonction MainWndProc
	
process_image_mmx	ENDP

	  END
; IMAGE.ASM
;
; MI01 - TP Assembleur 2 � 5
;
; R�alise le traitement d'une image 32 bits. 

.686
.MODEL FLAT, C

.DATA

.CODE

; **********************************************************************
; Sous-programme _process_image_asm 
; 
; R�alise le traitement d'une image 32 bits.
;
; Entr�es sur la pile : Largeur de l'image (entier 32 bits)
;			Hauteur de l'image (entier 32 bits)
;			Pointeur sur l'image source (d�pl. 32 bits)
;			Pointeur sur l'image tampon 1 (d�pl. 32 bits)
;			Pointeur sur l'image tampon 2 (d�pl. 32 bits)
;			Pointeur sur l'image finale (d�pl. 32 bits)
; **********************************************************************

PUBLIC		process_image_asm

process_image_asm	PROC NEAR		; Point d'entr�e du sous programme
		
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
		
traitement:
				
		MOV EAX, [ESI + ECX*4]		;8 bits plus faible est B (bit 0-7)
		MOVZX EDX,AL				
		IMUL EDX, 1Dh				; B' = 0.114*B
		MOV EBX, EDX				; I <= B'
		
		SHR EAX,8					;8 bits plus faible est V (bit 8-15)
		MOVZX EDX,AL	
		IMUL EDX, 96h				; V' = 0.587*V
		ADD EBX, EDX				; I <= I + V'

		SHR EAX,8					;8 bits plus faible est R (bit 16-23)
		MOVZX EDX,AL	
		IMUL EDX, 4Ch				; R' = 0.299*R
		ADD EBX, EDX				; I <= I + R'
		
		SHR EBX, 8					; d�calage de 8 bits vers la droite de EBX (I est alors l'octet de poids faible)
		MOV EAX, EBX				; mettre I dans les bits 0 � 7 (composante B) de EAX 
		SHL EAX, 8					; d�calage de 8 bits vers la droite de EBX (I est alors l'octet de 8 bits � 15 bits)
		ADD EBX, EAX				; mettre I dans les bits 8 � 15 (composante V) de EAX  
		SHL EAX, 8					; d�calage de 8 bits vers la droite de EBX (I est alors l'octet de 16 bits � 23 bits)
		ADD EBX, EAX				; mettre I dans les bits 16 � 23 (composante R) de EAX 

		
		MOV [EDI + ECX*4], EBX		; sauvegarder dans le destination
		DEC ECX						; d�cr�menter ECX pour passer au pixel pr�c�dent
		JG traitement				; si ECX>0 (dernier pixel non atteint), on recommence la boucle "traitement"

	
		
		
		POP EDX
		POP EAX						
		
		
		;*****************************************************************
		;*****************************************************************
      
			
fin:		
		pop     edi
		pop     esi
		pop     ebx

		pop     ebp

		ret			                ; Retour � la fonction MainWndProc
	
process_image_asm	ENDP

	  END
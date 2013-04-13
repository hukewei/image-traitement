/*  Atelier Photo - Travaux Pratiques UV MI01
 Copyright (C) 2005, 2012 S. Bonnet, Universit� de Technologie de Compi�gne

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include "appli_r.h"

/*
 * Constantes symboliques
 */
 
/* Identifiants des menus */
#define IDM_FILE 0
#define IDM_FILEOPEN 10
#define IDM_APPEXIT 11
#define IDM_SHOW 1
#define IDM_SHOWSOURCE 21
#define IDM_SHOWTEMP_1 22
#define IDM_SHOWTEMP_2 23
#define IDM_SHOWDEST 24
#define IDM_PROCESS 2
#define IDM_LAUNCH_ASM 30
#define IDM_LAUNCH_C 31
#define IDM_LAUNCH_MMX 32
#define IDM_SET_REPEAT 33


/*
 * Structures de donn�es
 */

/* ProcessedBitmap : bitmap 32 bits (0 R G B) utilis� pour le traitement */
typedef struct {
  UINT biWidth;		/* Largeur */
  UINT biHeight;    /* Hauteur */
  BYTE *pBits;		/* Pixels  */ 
  HBITMAP hBitmap;  /* Handle pour l'affichage */
} ProcessedBitmap;

/* Structure d'association Menu<->Message d'aide */
typedef struct {
  UINT uId;
  LPSTR lpszText;
} MenuDescription;

/*
 * Variables globales 
 */

/* Messages d'aide des menus */
MenuDescription MenuDescriptions[] = {
  {IDM_FILEOPEN, TEXT("Charger l'image bitmap source")},
  {IDM_APPEXIT, TEXT("Quitter l'application")},
  {IDM_SHOWSOURCE, TEXT("Afficher l'image source")},
  {IDM_SHOWTEMP_1, TEXT("Afficher l'image temporaire 1")},
  {IDM_SHOWTEMP_2, TEXT("Afficher l'image temporaire 2")},
  {IDM_SHOWDEST, TEXT("Afficher l'image finale")},
  {IDM_LAUNCH_ASM, TEXT("Lancer l'impl�mentation assembleur du traitement")},
  {IDM_LAUNCH_C, TEXT("Lancer l'impl�mentation C du traitement")},
  {IDM_SET_REPEAT, TEXT("Programmer le nombre de r�p�titions du traitement")},
  {IDM_LAUNCH_MMX, TEXT("Lancer l'impl�mentation MMX du traitement")},
  {0, ""}};

/* Titre de l'application */
TCHAR szAppName[] = TEXT("Atelier photo");

/* Handle des fen�tres */
HWND hwndMain;
HWND hwndStatus;

/* Fr�quence d'incr�mentation des compteurs de performances (hz) */
LARGE_INTEGER PerformanceFrequency;

/* Nombre de r�p�titions du traitement */
UINT nRepeats = 1;

/* Bitmaps de travail */
ProcessedBitmap* Bitmaps[4] = {NULL, NULL, NULL, NULL};

/* Num�ro du bitmap affich� */
UINT iVisibleBitmap = 0;

/*
 * Fonctions
 */

/* D�clarations externes */
extern UINT __cdecl process_image_asm(UINT biWidth, UINT biHeight, BYTE *img_src, 
                              BYTE *img_temp1, BYTE *img_temp2, BYTE *img_dest);

extern UINT __cdecl process_image_mmx(UINT biWidth, UINT biHeight, BYTE *img_src, 
                              BYTE *img_temp1, BYTE *img_temp2, BYTE *img_dest);

LRESULT CALLBACK MainWndProc (HWND, UINT, WPARAM, LPARAM);

/* process_image_C
 *
 * Fonction de traitement r�alisant le m�me traitement que le sous-programme 
 * assembleur utilis�e � titre de comparaison.
 *
 */
UINT __cdecl process_image_C(UINT biWidth, UINT biHeight, BYTE *img_src, 
                     BYTE *img_temp1, BYTE *img_temp2, BYTE *img_dest) {
  unsigned int c, n;
  unsigned int x, y;
  int sum;

  n = biWidth * biHeight * 4;

  for (c = 0; c < n; c = c + 4) {
    img_temp1[c] = (char) ((((int) img_src[c + 2]) * 0x4c 
        + ((int) img_src[c + 1] * 0x96) 
        + ((int) img_src[c] * 0x1d)) >> 8); 
  }

  /* D�tecteur de contours de Sobel */
  n = biWidth * 4;

  for(y = 0; y < biHeight - 2; y++) {
  for(x=0; x < (biWidth - 2) * 4; x = x + 4) {
      sum = 255 
          -(abs(- (int)((*(img_temp1 + (x + y * n))))
                + (int)((*(img_temp1 + (x + 8 + y * n))))
                - 2 * ((int)((*(img_temp1 + (x + (y + 1 )* n)))))
                + 2 * ((int)((*(img_temp1 + (x + 8 + (y + 1) * n)))))
                - ((int)((*(img_temp1 + (x + (y + 2) * n)))))
                + ((int)((*(img_temp1 + (x + 8 + (y + 2) * n))))))
          + abs((int)((*(img_temp1 + (x + y * n))))
                + 2 * (int)((*(img_temp1 + (x + 4 + y * n))))
                + (int)((*(img_temp1 + (x + 8 + y *n))))
                - (int)((*(img_temp1 + (x + (y + 2) * n))))
                - 2 * (int)((*(img_temp1 + (x + 4 + (y + 2) * n))))
                - (int)((*(img_temp1 + (x + 8 + (y + 2) * n))))));
        
      if(sum < 0) {
        sum = 0;
      }

      *((int *) (img_temp2 + (x + 4 + (y + 1) * n))) = sum | (sum << 8) | (sum << 16);
    }
  }

  n = biWidth * biHeight * 4;

  for (c = 0; c < n; c = c + 4) {
    if (img_temp2[c] > 150) {
      *((int*) (img_dest + c)) = 0xffffff;
    } else {
      *((int*) (img_dest + c)) = 0;
    }
  }

  return(0);
}

/* CreateMenuBar
 *
 * Cr�e la barre de menu et retourne son handle.
 *
 */
HMENU CreateMenuBar(void) {
  HMENU hMenu;
  HMENU hSubMenu;
  
  hMenu = CreateMenu();
  hSubMenu = CreateMenu();
  
  AppendMenu(hSubMenu, MF_STRING, IDM_FILEOPEN, "&Ouvrir l'image source...");
  AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hSubMenu, MF_STRING, IDM_APPEXIT, "&Quitter");

  AppendMenu(hMenu, MF_POPUP, (UINT_PTR) hSubMenu, "&Fichier");
  
  hSubMenu = CreateMenu();
  
  AppendMenu(hSubMenu, MF_STRING, IDM_SHOWSOURCE, "Image &source");
  AppendMenu(hSubMenu, MF_STRING, IDM_SHOWTEMP_1, "Image interm�diaire &1");
  AppendMenu(hSubMenu, MF_STRING, IDM_SHOWTEMP_2, "Image interm�diaire &2");
  AppendMenu(hSubMenu, MF_STRING, IDM_SHOWDEST, "Image &finale");
  
  AppendMenu(hMenu, MF_POPUP, (UINT_PTR) hSubMenu, "&Affichage");
  
  hSubMenu = CreateMenu();
 
  AppendMenu(hSubMenu, MF_STRING, IDM_LAUNCH_ASM, 
             "Lancer le traitement &assembleur");
  AppendMenu(hSubMenu, MF_STRING, IDM_LAUNCH_MMX, 
             "Lancer le traitement &MMX");
  AppendMenu(hSubMenu, MF_STRING, IDM_LAUNCH_C, "Lancer le traitement &C");
  AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hSubMenu, MF_STRING, IDM_SET_REPEAT, "&R�p�titions...");
  
  AppendMenu(hMenu, MF_POPUP, (UINT_PTR) hSubMenu, "&Traitement");
  
  return(hMenu);
}

/* EnableProcessMenu
 *
 * Active/D�sactive le menu Traitements
 *
 */
void EnableProcessMenu(BOOL enabled) {
  HMENU hMenu;
  
  hMenu = GetMenu(hwndMain);
  
  if (enabled) {
    EnableMenuItem(hMenu, IDM_PROCESS, MF_BYPOSITION | MF_ENABLED);
  } else {
    EnableMenuItem(hMenu, IDM_PROCESS, MF_BYPOSITION | MF_GRAYED);
  }
  DrawMenuBar(hwndMain);
}

/* CreateStatus
 *
 * Cr�e la barre d'�tat et retourne son handle.
 *
 */
HWND CreateStatus(HINSTANCE hInstance, HWND hwndParent)
{
  return(CreateWindowEx(0, STATUSCLASSNAME, (LPCTSTR) NULL,         
                        SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, 
                        hwndParent, (HMENU) 1, hInstance, NULL));
}             

/* UpdateStatus
 *
 * Met � jour le texte de la barre d'�tat
 *
 */ 
void UpdateStatus(LPSTR lpszText)
{
  SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM) lpszText);
}

/* GetBitmapFileName
 *
 * Affiche une bo�te de dialogue de s�lection de fichiers image .BMP.
 *
 * Retourne le chemin d'acc�s complet au fichier si s�lection r�ussie, NULL sinon.
 *
 */
LPSTR GetBitmapFileName(HWND hwndParent)
{
  static int first = 1;
    
  static OPENFILENAME ofn;
  static TCHAR szFileName[MAX_PATH], szTitleName[MAX_PATH];
  static TCHAR szFilter[] = TEXT ("Fichiers bitmap (*.BMP)\0*.BMP\0\0");
  
  if (first) {
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = szTitleName;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL;
    ofn.Flags  = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = TEXT ("bmp");
    ofn.lCustData  = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;
    first = !first;
  }
      
  ofn.hwndOwner = hwndParent;
  
  if (GetOpenFileName(&ofn)) {
    return(ofn.lpstrFile);
  }
  return(NULL);
}

/* CreateProcessedBitmap
 *
 * Cr�e un bitmap de taille biHeight x biWidth, chaque pixel est repr�sent� par un 
 * entier de 32 bits : 0 R G B.
 *
 * Retourne un pointeur sur le nouveau bitmap, NULL si �chec.
 */
ProcessedBitmap *CreateProcessedBitmap(UINT biWidth, UINT biHeight) {
  BITMAPINFOHEADER bmih;
  BYTE *pBits;
  HBITMAP hBitmap;
  ProcessedBitmap *bitmap;
  
  bmih.biSize = sizeof(BITMAPINFOHEADER);
  bmih.biWidth = biWidth;
  bmih.biHeight = biHeight;
  bmih.biPlanes = 1;
  bmih.biBitCount = 32;
  bmih.biCompression = BI_RGB;
  bmih.biSizeImage = 0;
  bmih.biXPelsPerMeter = 0;
  bmih.biYPelsPerMeter = 0;
  bmih.biClrUsed = 0;
  bmih.biClrImportant = 0;
  
  hBitmap = CreateDIBSection(NULL, (CONST BITMAPINFO *) &bmih, 0, 
                            (VOID **) &pBits, NULL, 0);
  
  if (hBitmap != NULL) {
    bitmap = (ProcessedBitmap *) _aligned_malloc(sizeof(ProcessedBitmap), 64);
    bitmap->biHeight = biHeight;
    bitmap->biWidth = biWidth;
    bitmap->hBitmap = hBitmap;
    bitmap->pBits = pBits;
    return(bitmap);
  } else {
    return(NULL);
  }
}

/* DeleteProcessedBitmap
 *
 * Lib�re les ressources associ�es � un bitmap cr�� par CreateProcessedBitmap
 *
 */
void DeleteProcessedBitmap(ProcessedBitmap *bitmap) {
  if (bitmap->hBitmap != NULL) {
    DeleteObject(bitmap->hBitmap);
  }
  _aligned_free(bitmap);
}

/* LoadImageFile
 * 
 * Charge un fichier bitmap .BMP 24 ou 32 bits et le convertit dans le format
 * de traitement (32 bits).
 *
 * Retourne un pointeur vers le nouveau bitmap si r�ussite, NULL si �chec.
 * 
 */
ProcessedBitmap *LoadImageFile(LPSTR lpszFileName)
{
  BOOL bSuccess ;
  DWORD dwFileSize, dwHighSize, dwBytesRead ;
  UINT biWidth, biHeight;
  UINT biBitCount;
  HANDLE hFile ;
  BITMAPFILEHEADER *pbmfh ;
  BITMAPCOREHEADER *pbmch;
  BITMAPINFOHEADER *pbmih;
  BYTE *pBits;
  ProcessedBitmap *bitmap = NULL;
  UINT nPadding, iSrcPel, iDestPel, i, j;
    
  /* Ouvrir le fichier en lecture */
  hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                     OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    return(NULL);
  }

  /* R�cup�rer la taille du fichier */
  dwFileSize = GetFileSize (hFile, &dwHighSize);

  if (dwHighSize) {
    CloseHandle(hFile);
    return(NULL);
  }

  /* Allouer un buffer pour le fichier */
  pbmfh = malloc(dwFileSize);
  if (!pbmfh) {
    CloseHandle(hFile);
    return(NULL);
  }
  
  /* Copier le fichier dans le buffer */
  bSuccess = ReadFile(hFile, pbmfh, dwFileSize, &dwBytesRead, NULL);
  CloseHandle(hFile);
  
  /* V�rifier que c'est bien un fichier au format BMP */
  if (bSuccess && (dwBytesRead == dwFileSize)         
               && (pbmfh->bfType == * (WORD *) "BM") 
               && (pbmfh->bfSize == dwFileSize)) {
    /* Convertir le bitmap */

    /* R�cup�rer la taille et la profondeur de couleurs dans l'en-t�te */
    pbmih = (BITMAPINFOHEADER *) (((BYTE *) pbmfh) + sizeof (BITMAPFILEHEADER));
    if (pbmih->biSize == sizeof(BITMAPCOREHEADER)) {
      pbmch = (BITMAPCOREHEADER *) (((BYTE *)pbmfh) + sizeof (BITMAPFILEHEADER));
      biWidth = pbmch->bcWidth;
      biHeight = pbmch->bcHeight;
      biBitCount = pbmch->bcBitCount;
    } else {
      biWidth = pbmih->biWidth;
      biHeight = pbmih->biHeight;
      biBitCount = pbmih->biBitCount;
    }
      
    /* R�cup�rer l'adresse du premier pixel */
    pBits = (BYTE *) (((BYTE *)pbmfh) + pbmfh->bfOffBits);

    /* On ne traite que les bitmaps 24 ou 32 bits */
    if (biBitCount == 24) {
      bitmap = CreateProcessedBitmap(biWidth, biHeight);
      if (bitmap) {
        /* Copier les bits de l'image, conversion 24->32 bits */
        nPadding = (4 - ((biWidth * 3) & 3)) & 3;
        iSrcPel = 0;
        iDestPel = 0;
        memset(bitmap->pBits, 0, biWidth * biHeight * 4);

        for (i = 0; i < biHeight; i++) {
          for (j = 0; j < biWidth; j++) { 
            memcpy(bitmap->pBits + iDestPel, pBits + iSrcPel, 3);
            iSrcPel = iSrcPel + 3;
            iDestPel = iDestPel + 4;
          }
        iSrcPel += nPadding;
        }
      } 
    } else if (biBitCount == 32) {
      bitmap = CreateProcessedBitmap(biWidth, biHeight);
      if (bitmap) {
        /* Copier les bits de l'image directement */
        memcpy(bitmap->pBits, pBits, (pbmfh->bfSize - pbmfh->bfOffBits));
      }         
    }
  }
  
  /* Lib�rer le buffer de fichier */
  free(pbmfh);

  return(bitmap);      
}

/* RepeatDlgProc
 *
 * Proc�dure de fen�tre de la bo�te de saisie du nombre de r�p�titions.
 *
 */
BOOL CALLBACK RepeatDlgProc(HWND hDlg, UINT message, WPARAM wParam, 
                            LPARAM lParam)
{
  UINT repeats;
  BOOL bParsedOK;
  
  switch (message) {
    case WM_INITDIALOG :
      SetDlgItemInt(hDlg, EDT_NREPEATS, nRepeats, FALSE);
      return(TRUE);
          
    case WM_COMMAND :
      switch (LOWORD (wParam)) {
        case IDOK :
          repeats = GetDlgItemInt(hDlg, EDT_NREPEATS, &bParsedOK, FALSE);
          if (bParsedOK) {
            nRepeats = repeats;
            EndDialog(hDlg, 0);
          }
          return(TRUE);     
        case IDCANCEL :
          EndDialog (hDlg, 0);
          return(TRUE);
      }
      break ;
  }
  return FALSE ;
}

/* MainWndProc
 *
 * Proc�dure de fen�tre de la fen�tre principale
 *
 * Traite les messages envoy�s par Windows.
 *
 */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam)
{
  static HINSTANCE hInstance;
  HDC hdc, hdcMem;
  PAINTSTRUCT ps ;
  RECT rect, rcBitmap ;
  UINT i;
  LPSTR lpszFile;
  ProcessedBitmap*mybm;
  TCHAR buffer[128];
  LARGE_INTEGER beginProcess;
  LARGE_INTEGER endProcess;
  LARGE_INTEGER processTime;
  BOOL bLaunchMMX = FALSE;
         
  switch (message) {
    case WM_CREATE:
      hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
      return(0); 
      
    case WM_PAINT:
      hdc = BeginPaint (hwnd, &ps) ;
      mybm = Bitmaps[iVisibleBitmap];
        
      if (mybm) {
        rcBitmap.left = 0;
        rcBitmap.top = 0;
        rcBitmap.right = mybm->biWidth;
        rcBitmap.bottom = mybm->biHeight;
           
        if (IntersectRect(&rect, &rcBitmap, &ps.rcPaint)) {
            hdcMem = CreateCompatibleDC (hdc);
            SelectObject (hdcMem, mybm->hBitmap);
            BitBlt (hdc,    rect.left, rect.top, rect.right, rect.bottom, 
            hdcMem, rect.left, rect.top, SRCCOPY);
            DeleteDC (hdcMem) ;
        }             
      } else {
        GetClientRect (hwnd, &rect) ;
        DrawText(hdc, TEXT ("Aucune image"), -1, &rect,
                 DT_SINGLELINE | DT_CENTER | DT_VCENTER) ;
      } 
      EndPaint (hwnd, &ps) ;
      return(0);
  
    case WM_DESTROY:
      PostQuitMessage(0);
      return(0);
    
    case WM_MENUSELECT:
      i = 0;
      if (HIWORD(wParam) != 0xffff) {           
        while((MenuDescriptions[i].uId != 0) 
               && (MenuDescriptions[i].uId != (UINT) LOWORD(wParam))) { 
          i++;
        }
        UpdateStatus(MenuDescriptions[i].lpszText);       
      } else {
        UpdateStatus("");
      }
      break;
      
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
        case IDM_FILEOPEN:
          if ((lpszFile = GetBitmapFileName(hwnd)) != NULL) {
            mybm = LoadImageFile(lpszFile);
            if (mybm == NULL) {
              MessageBox(hwnd, 
              TEXT("Impossible d'ouvrir le fichier image.\n"),
              TEXT("Erreur"),
              MB_ICONERROR | MB_OK);
            } else {
              iVisibleBitmap = 0;
              for (i = 0; i < 4; i++) {
                if (Bitmaps[i] != NULL) {
                  DeleteProcessedBitmap(Bitmaps[i]);
                }
              }
              Bitmaps[iVisibleBitmap] = mybm;
              Bitmaps[1] = CreateProcessedBitmap(mybm->biWidth, mybm->biHeight);
              Bitmaps[2] = CreateProcessedBitmap(mybm->biWidth, mybm->biHeight);
              Bitmaps[3] = CreateProcessedBitmap(mybm->biWidth, mybm->biHeight);
              EnableProcessMenu(TRUE);
              sprintf_s(buffer, sizeof(buffer), "%s - %s", szAppName, "Image source");
              SetWindowText(hwnd, buffer);
              InvalidateRect(hwnd, NULL, TRUE);
            }
          }
          break;
          
        case IDM_APPEXIT:           
          SendMessage(hwnd, WM_CLOSE, 0, 0);
          break;
          
        case IDM_SHOWSOURCE:
          sprintf_s(buffer, sizeof(buffer), "%s - %s", &szAppName, "Image source");
          SetWindowText(hwnd, buffer);
          iVisibleBitmap = LOWORD(wParam) - IDM_SHOWSOURCE;
          InvalidateRect(hwnd, NULL, TRUE);
          break;
          
        case IDM_SHOWTEMP_1:
          sprintf_s(buffer, sizeof(buffer), "%s - %s", &szAppName, "Image interm�diaire 1");
          SetWindowText(hwnd, buffer);
          iVisibleBitmap = LOWORD(wParam) - IDM_SHOWSOURCE;
          InvalidateRect(hwnd, NULL, TRUE);
          break;
          
        case IDM_SHOWTEMP_2:
          sprintf_s(buffer, sizeof(buffer), "%s - %s", &szAppName, "Image interm�diaire 2");
          SetWindowText(hwnd, buffer);
          iVisibleBitmap = LOWORD(wParam) - IDM_SHOWSOURCE;
          InvalidateRect(hwnd, NULL, TRUE);
          break;
          
        case IDM_SHOWDEST:
          sprintf_s(buffer, sizeof(buffer), "%s - %s", &szAppName, "Image finale");
          SetWindowText(hwnd, buffer);
          iVisibleBitmap = LOWORD(wParam) - IDM_SHOWSOURCE;
          InvalidateRect(hwnd, NULL, TRUE);
          break;
          
        case IDM_LAUNCH_MMX:
          bLaunchMMX = TRUE;
        case IDM_LAUNCH_ASM:
          QueryPerformanceCounter(&beginProcess);
          for (i = 0; i < nRepeats; i++) {
            if (!bLaunchMMX) {
              process_image_asm(Bitmaps[0]->biWidth, 
                              Bitmaps[0]->biHeight, 
                              Bitmaps[0]->pBits,
                              Bitmaps[1]->pBits,
                              Bitmaps[2]->pBits,
                              Bitmaps[3]->pBits);
            } else {
              process_image_mmx(Bitmaps[0]->biWidth, 
                              Bitmaps[0]->biHeight, 
                              Bitmaps[0]->pBits,
                              Bitmaps[1]->pBits,
                              Bitmaps[2]->pBits,
                              Bitmaps[3]->pBits);
            }
          }
          QueryPerformanceCounter(&endProcess);

          processTime.QuadPart = endProcess.QuadPart - beginProcess.QuadPart;

          if (PerformanceFrequency.QuadPart != 0) {
            sprintf_s(buffer, sizeof(buffer), "Temps de calcul pour %d it�ration(s) : \n"
                      "Total : %f ms \n Par it�ration : %f ms",
                      nRepeats,
                      (((double) processTime.QuadPart) * 1000.0) 
                      / ((double) PerformanceFrequency.QuadPart),
                      (((double) processTime.QuadPart) * 1000.0) 
                      / ((double) PerformanceFrequency.QuadPart) 
                      / ((double) nRepeats));
          } else {
            sprintf_s(buffer, sizeof(buffer), "Pas de support mat�riel pour l'�valuation du"
                      " temps de calcul.");
          }
          MessageBox(hwnd, buffer, "Temps de calcul", 
                     MB_ICONINFORMATION |MB_OK);             
          InvalidateRect(hwnd, NULL, TRUE);
          break;
        case IDM_LAUNCH_C:
          QueryPerformanceCounter(&beginProcess);
          for (i = 0; i < nRepeats; i++) {
            process_image_C(Bitmaps[0]->biWidth,
                            Bitmaps[0]->biHeight,
                            Bitmaps[0]->pBits,
                            Bitmaps[1]->pBits,
                            Bitmaps[2]->pBits,
                            Bitmaps[3]->pBits);
          }
          
          QueryPerformanceCounter(&endProcess);
          processTime.QuadPart = endProcess.QuadPart - beginProcess.QuadPart;

          if (PerformanceFrequency.QuadPart != 0) {
            sprintf_s(buffer, sizeof(buffer), "Temps de calcul pour %d it�ration(s) : \n"
                            "Total : %f ms \n Par it�ration : %f ms",
                    nRepeats,
                    (((double) processTime.QuadPart) * 1000.0) 
                    / ((double) PerformanceFrequency.QuadPart),
                    (((double) processTime.QuadPart) * 1000.0) 
                    / ((double) PerformanceFrequency.QuadPart) 
                    / ((double) nRepeats));
          } else {
            sprintf_s(buffer, sizeof(buffer), "Pas de support mat�riel pour l'�valuation du temps"
                            "de calcul.");
          }
          MessageBox(hwnd, buffer, "Temps de calcul", MB_ICONINFORMATION |MB_OK);             
          InvalidateRect(hwnd, NULL, TRUE);
          break;
          
        case IDM_SET_REPEAT:
          DialogBox(hInstance, TEXT("DLG_REPEAT"), hwnd, (DLGPROC)RepeatDlgProc);
          break ;
      }
      return(0);
      
    case WM_SIZE:
      SendMessage(hwndStatus, WM_SIZE, wParam, lParam);
      if (Bitmaps[iVisibleBitmap] == NULL) {
        InvalidateRect(hwnd, NULL, TRUE);
      }
      return(0);
  }
  return DefWindowProc (hwnd, message, wParam, lParam);
}

/* WinMain
 *
 * Fonction de d�marrage du programme
 *
 */
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow) {
  static MSG  msg;
  WNDCLASS wndclass;

  InitCommonControls();

  wndclass.style = 0; //CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc = MainWndProc ;
  wndclass.cbClsExtra = 0 ;
  wndclass.cbWndExtra = 0 ;
  wndclass.hInstance = hInstance ;
  wndclass.hIcon = LoadIcon (NULL, IDI_APPLICATION) ;
  wndclass.hCursor = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
  wndclass.lpszMenuName = NULL ;
  wndclass.lpszClassName = TEXT("TPMI01MAIN");

  if (!RegisterClass (&wndclass)) {
    MessageBox(NULL, TEXT("Ce programme requiert Windows 2000 ou sup�rieur"), 
               szAppName, MB_ICONERROR);
    return(0);
  }

  hwndMain = CreateWindow (TEXT("TPMI01MAIN"), szAppName, WS_OVERLAPPEDWINDOW,        
                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,              
                           CW_USEDEFAULT, NULL, CreateMenuBar(), hInstance,                  
                           NULL) ;                     

  hwndStatus = CreateStatus(hInstance, hwndMain);
  EnableProcessMenu(FALSE);
  ShowWindow (hwndMain, iCmdShow);
  UpdateWindow (hwndMain);

  PerformanceFrequency.QuadPart = 0;
  if (!QueryPerformanceFrequency(&PerformanceFrequency)) { 
    MessageBox(hwndMain, "Ce syst�me ne dispose pas de support pour la mesure"
               " de performance.\nLes mesures de performances ne seront pas"
               " activ�es.", szAppName, MB_ICONWARNING | MB_OK);
  }      

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return((int) msg.wParam);
}

// This file is part of the RISC OS Toolkit (RTK).
// Copyright © 2003 Graham Shaw.
// Distribution and use are subject to the GNU Lesser General Public License,
// a copy of which may be found in the file !RTK.Copyright.

#ifndef _LIBPKG_SWI_OS
#define _LIBPKG_SWI_OS

// Cut down version to use with LibPkg created by Alan Buckley
// to remove the RTK dependency.

namespace pkg {
namespace swi {

const int OS_WriteC             =0x00;
const int OS_WriteS             =0x01;
const int OS_Write0             =0x02;
const int OS_NewLine            =0x03;
const int OS_ReadC              =0x04;
const int OS_CLI                =0x05;
const int OS_Byte               =0x06;
const int OS_Word               =0x07;
const int OS_File               =0x08;
const int OS_Args               =0x09;
const int OS_BGet               =0x0A;
const int OS_BPut               =0x0B;
const int OS_GBPB               =0x0C;
const int OS_Find               =0x0D;
const int OS_ReadLine           =0x0E;
const int OS_Control            =0x0F;
const int OS_GetEnv             =0x10;
const int OS_Exit               =0x11;
const int OS_SetEnv             =0x12;
const int OS_IntOn              =0x13;
const int OS_IntOff             =0x14;
const int OS_CallBack           =0x15;
const int OS_EnterOS            =0x16;
const int OS_BreakPt            =0x17;
const int OS_BreakCtrl          =0x18;
const int OS_UnusedSWI          =0x19;
const int OS_UpdateMEMC         =0x1A;
const int OS_SetCallBack        =0x1B;
const int OS_Mouse              =0x1C;
const int OS_Heap               =0x1D;
const int OS_Module             =0x1E;
const int OS_Claim              =0x1F;
const int OS_Release            =0x20;
const int OS_ReadUnsigned       =0x21;
const int OS_GenerateEvent      =0x22;
const int OS_ReadVarVal         =0x23;
const int OS_SetVarVal          =0x24;
const int OS_GSInit             =0x25;
const int OS_GSRead             =0x26;
const int OS_GSTrans            =0x27;
const int OS_BinaryToDecimal    =0x28;
const int OS_FSControl          =0x29;
const int OS_ChangeDynamicArea  =0x2A;
const int OS_GenerateError      =0x2B;
const int OS_ReadEscapeState    =0x2C;
const int OS_EvaluateExpression =0x2D;
const int OS_SpriteOp           =0x2E;
const int OS_ReadPalette        =0x2F;
const int OS_ServiceCall        =0x30;
const int OS_ReadVduVariables   =0x31;
const int OS_ReadPoint          =0x32;
const int OS_UpCall             =0x33;
const int OS_CallAVector        =0x34;
const int OS_ReadModeVariable   =0x35;
const int OS_RemoveCursors      =0x36;
const int OS_RestoreCursors     =0x37;
const int OS_SWINumberToString  =0x38;
const int OS_SWINumberFromString=0x39;
const int OS_ValidateAddress    =0x3A;
const int OS_CallAfter          =0x3B;
const int OS_CallEvery          =0x3C;
const int OS_RemoveTickerEvent  =0x3D;
const int OS_InstallKeyHandler  =0x3E;
const int OS_CheckModeValid     =0x3F;
const int OS_ChangeEnvironment  =0x40;
const int OS_ClaimScreenMemory  =0x41;
const int OS_ReadMonotonicTime  =0x42;
const int OS_SubstituteArgs     =0x43;
const int OS_PrettyPrint        =0x44;
const int OS_Plot               =0x45;
const int OS_WriteN             =0x46;
const int OS_AddToVector        =0x47;
const int OS_WriteEnv           =0x48;
const int OS_ReadArgs           =0x49;
const int OS_ReadRAMFsLimits    =0x4A;
const int OS_ClaimDeviceVector  =0x4B;
const int OS_ReleaseDeviceVector=0x4C;
const int OS_DelinkApplication  =0x4D;
const int OS_RelinkApplication  =0x4E;
const int OS_HeapSort           =0x4F;
const int OS_ExitAndDie         =0x50;
const int OS_ReadMemMapInfo     =0x51;
const int OS_ReadMemMapEntries  =0x52;
const int OS_SetMemMapEntries   =0x53;
const int OS_AddCallBack        =0x54;
const int OS_ReadDefaultHandler =0x55;
const int OS_SetECFOrigin       =0x56;
const int OS_SerialOp           =0x57;
const int OS_ReadSysInfo        =0x58;
const int OS_Confirm            =0x59;
const int OS_ChangedBox         =0x5A;
const int OS_CRC                =0x5B;
const int OS_ReadDynamicArea    =0x5C;
const int OS_PrintChar          =0x5D;
const int OS_ChangeRedirection  =0x5E;
const int OS_RemoveCallBack     =0x5F;
const int OS_FindMemMapEntries  =0x60;
const int OS_SetColour          =0x61;
const int OS_Pointer            =0x64;
const int OS_ScreenMode         =0x65;
const int OS_DynamicArea        =0x66;
const int OS_Memory             =0x68;
const int OS_ClaimProcessorVector=0x69;
const int OS_Reset              =0x6A;
const int OS_MMUControl         =0x6B;
const int OS_ResyncTime         =0x6C;
const int OS_PlatformFeatures   =0x6D;
const int OS_SynchroniseCodeAreas=0x6E;
const int OS_CallASWI           =0x6F;
const int OS_AMBControl         =0x70;
const int OS_CallASWIR12        =0x71;
const int OS_SpecialControl     =0x72;
const int OS_EnterUSR32         =0x73;
const int OS_EnterUSR26         =0x74;

const int OS_ConvertStandardDateAndTime=0xC0;
const int OS_ConvertDateAndTime =0xC1;
const int OS_ConvertHex1        =0xD0;
const int OS_ConvertHex2        =0xD1;
const int OS_ConvertHex4        =0xD2;
const int OS_ConvertHex6        =0xD3;
const int OS_ConvertHex8        =0xD4;
const int OS_ConvertCardinal1   =0xD5;
const int OS_ConvertCardinal2   =0xD6;
const int OS_ConvertCardinal3   =0xD7;
const int OS_ConvertCardinal4   =0xD8;
const int OS_ConvertInteger1    =0xD9;
const int OS_ConvertInteger2    =0xDA;
const int OS_ConvertInteger3    =0xDB;
const int OS_ConvertInteger4    =0xDC;
const int OS_ConvertBinary1     =0xDD;
const int OS_ConvertBinary2     =0xDE;
const int OS_ConvertBinary3     =0xDF;
const int OS_ConvertBinary4     =0xE0;
const int OS_ConvertSpacedCardinal1=0xE1;
const int OS_ConvertSpacedCardinal2=0xE2;
const int OS_ConvertSpacedCardinal3=0xE3;
const int OS_ConvertSpacedCardinal4=0xE4;
const int OS_ConvertSpacedInteger1=0xE5;
const int OS_ConvertSpacedInteger2=0xE6;
const int OS_ConvertSpacedInteger3=0xE7;
const int OS_ConvertSpacedInteger4=0xE8;
const int OS_ConvertFixedNetStation=0xE9;
const int OS_ConvertNetStation  =0xEA;
const int OS_ConvertFixedFileSize=0xEB;
const int OS_ConvertFileSize    =0xEC;
const int OS_WriteI             =0x100;

const int ModeFlags     =0;
const int ScrRCol       =1;
const int ScrBRow       =2;
const int NColour       =3;
const int XEigFactor    =4;
const int YEigFactor    =5;
const int LineLength    =6;
const int ScreenSize    =7;
const int YShftFactor   =8;
const int Log2BPP       =9;
const int Log2BPC       =10;
const int XWindLimit    =11;
const int YWindLimit    =12;

} /* namespace swi */
} /* namespace pkg */

#endif

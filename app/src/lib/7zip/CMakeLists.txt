add_definitions("-D_7ZIP_ST")

add_library(7zip STATIC
     7zAlloc.c
     7zArcIn.c
     7zBuf.c
     7zBuf2.c
     7zCrc.c
     7zCrcOpt.c
     7zDec.c
     7zFile.c
     7zStream.c
     Aes.c
     AesOpt.c
     Alloc.c
     Bcj2.c
     Bcj2Enc.c
     Bra.c
     Bra86.c
     BraIA64.c
     CpuArch.c
     Delta.c
     DllSecur.c
     LzFind.c
     Lzma2Dec.c
     Lzma2Enc.c
     Lzma86Dec.c
     Lzma86Enc.c
     LzmaDec.c
     LzmaEnc.c
     LzmaLib.c
     MtCoder.c
     MtDec.c
     Ppmd7.c
     Ppmd7Dec.c
     Ppmd7Enc.c
     Sha256.c
     Sort.c
     Xz.c
     XzCrc64.c
     XzCrc64Opt.c
     XzDec.c
     XzEnc.c
     XzIn.c
)

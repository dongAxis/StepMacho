//
//  QHDefined.h
//  StepMacho
//
//  Created by Axis on 16/1/18.
//  Copyright © 2016年 Axis. All rights reserved.
//

#ifndef QHDefined_h
#define QHDefined_h

#undef QH_ISERROR
#define QH_ISERROR(errcode)     ((errcode)<QH_SUCCESS)

#pragma mark - error code
#undef ERRORCODE
#define ERRORCODE int

#define QH_SUCCESS 0
#define QH_MACHO_NOT_EXISTED            QH_SUCCESS-1
#define QH_MACHO_NOT_PARM_INVALID       QH_SUCCESS-2
#define QH_MACHO_FILE_OPEN_ERROR        QH_SUCCESS-3
#define QH_MACHO_FILE_READ_ERROR        QH_SUCCESS-4
#define QH_MACHO_NOT_CONTAIN_SEG        QH_SUCCESS-5
#define QH_MACHO_LLVM_NO_SECT           QH_SUCCESS-6
#define QH_MACHO_LLVM_ERROR             QH_SUCCESS-7
#define QH_MACHO_UNKNOWN_ARCH           QH_SUCCESS-8
#define QH_CREATE_DIR_FAILED            QH_SUCCESS-9
#define QH_FAILED                       QH_SUCCESS-10

#pragma mark code segment
#ifndef SET_ERRCODE_WITH_BREAK
#define SET_ERRCODE_WITH_BREAK(err_variable, err_code)  \
            {(err_variable) = (err_code);                \
            break;}
#endif

#ifndef SET_ERRCODE_WITH_RETURN
#define SET_ERRCODE_WITH_RETURN(err_variable, err_code)                 \
        do {                                                            \
            (err_variable) = (err_code);                                \
            return (err_variable);                                      \
        }while(0);
#endif

#undef LITTLE_ENDINE_32
#define LITTLE_ENDINE_32(x)                     \
                    ((x>>24) & 0x000000FF)  |   \
                    ((x>>8) & 0x0000FF00)   |   \
                    ((x<<24) & 0xFF000000)  |   \
                    ((x<<8) & 0x00FF0000)

#define CAPACIRY(x)     (sizeof((x))/sizeof((x[0])))

#define READ_FILE_WITH_FIXED(fp, array, offset)                 \
            do {                                                \
                lseek(fp, offset, SEEK_SET);                    \
                bzero(array, sizeof(array));                    \
                read(fp, array, CAPACIRY(array));    \
            }while(0);


#ifndef QHEXPORT
#define QHEXPORT __attribute__((visibility("default")))
#endif

#ifndef LLVM_SEG
#define LLVM_SEG "__LLVM"
#endif

#ifndef LLVM_BUNDLE
#define LLVM_BUNDLE "__bundle"
#endif


#endif /* QHDefined_h */

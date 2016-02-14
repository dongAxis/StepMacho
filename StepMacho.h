//
//  StepMacho.h
//  StepMacho
//
//  Created by Axis on 16/1/18.
//  Copyright © 2016年 Axis. All rights reserved.
//

#ifndef StepMacho_h
#define StepMacho_h

#include "QHDefined.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    enum archs
    {
        x32=0,
        x86_64,
        arm,
        arm64,
        arch_max
    };
    
    enum sub_archs
    {
        armv7_sub=0,
        armv7s_sub,
        arm64_sub,
        arch_sub_max
    };
    
    struct XarFileProperty
    {
        char macho_name[128];
        int num_arch;
        char work_dir[128];
        char path[arch_sub_max][128];
        char xar_path[arch_sub_max][128];         //file path
        char bitcode_path[arch_sub_max][128];
        char macho_path[arch_sub_max][128];
        char arch_sub[arch_sub_max][16];
    };
    
    ERRORCODE DumpMachoLLVMSec(const char* macho_path, const char * filename, const char* make_tpl_path);
    
#ifdef __cplusplus
}
#endif


#define    COLOR_END                        "\033[0m"
#define    FONT_COLOR_RED                   "\033[0;31m"
#define    FONT_COLOR_BLUE                  "\033[1;34m"

#define INFO(args...)                               \
        printf(FONT_COLOR_BLUE "[INFO]" COLOR_END); \
        printf(FONT_COLOR_BLUE args COLOR_END);   \
        printf("\n");

#define WARNING(args...)    \
        printf(FONT_COLOR_RED "[WARNING]" COLOR_END);printf(FONT_COLOR_RED args COLOR_END); printf("\n");

#endif /* StepMacho_h */

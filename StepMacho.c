//
//  StepMacho.c
//  StepMacho
//
//  Created by Axis on 16/1/18.
//  Copyright © 2016年 Axis. All rights reserved.
//
#include <mach/mach_types.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>

#include "StepMacho.h"

//ERRORCODE IsMachoFat(mach_vm_address_t *address)
//{
//    
//}
//typedef union
//{
//    struct mach_header      _32;
//    struct mach_header_64   _64;
//}MachOHeader;
//
//typedef union
//{
//    struct segment_command      _32;
//    struct segment_command_64   _64;
//}MachOSegCommand;

static char archs_str[arch_max][100] =
{
    "x32",
    "x86_64",
    "arm",
    "arm64",
};

static char archs_sub_str[arch_sub_max][100] =
{
    "armv7",
    "armv7s",
    "arm64"
};

static struct XarFileProperty info;

char *make_file_template="";

void mkdirs(char *muldir)
{
    unsigned long i,len;
    char str[512];
    strncpy(str, muldir, 512);
    len=strlen(str);
    for( i=0; i<len; i++ )
    {
        if( str[i]=='/' )
        {
            str[i] = '\0';
            if( access(str,0)!=0 )
            {
                mkdir( str, 0777 );
            }
            str[i]='/';
        }
    }
    if( len>0 && access(str,F_OK)!=0 )
    {
        mkdir( str, 0777 );
    }
    return;
}

enum sub_archs getSubArchs(cpu_type_t cpu, cpu_subtype_t sub_cpu)
{
    switch(cpu)
    {
        case CPU_TYPE_ARM:
        {
            switch(sub_cpu)
            {
                case CPU_SUBTYPE_ARM_V7:    return armv7_sub;
                case CPU_SUBTYPE_ARM_V7S:   return armv7s_sub;
            }
        }break;
        case CPU_TYPE_ARM64:
        {
            switch(sub_cpu)
            {
                case CPU_SUBTYPE_ARM64_ALL: return arm64_sub;
            }
        }break;
    }
    
    return arch_sub_max;
}

ERRORCODE executeSystem(char* cmd)
{
    FILE *fp_cmd = popen(cmd, "r");
    if(!fp_cmd) return QH_FAILED;
    
    char tmp[1024];
    while(fgets(tmp, sizeof(tmp), fp_cmd)!=NULL)
    {
        //just receive the output of cmd
    }
    
    pclose(fp_cmd);
//    sleep(1);
    return 0;
}

ERRORCODE readFileList(char *basePath, char ***bitcode, int *num)
{
    DIR *dir;
    struct dirent *ptr;
    int get_num = 0;
    
    //if the bitcode is not null, just allocate the memory
    if(bitcode!=NULL && *num>0)
    {
        get_num=0;
        *bitcode = (char**)malloc(sizeof(char**)*sizeof(num));
        if(*bitcode==NULL) return QH_FAILED;
        
        for(int i=0; i<*num; i++)
        {
            (*bitcode)[i] = malloc(sizeof(char)*32);
            if((*bitcode)[i]==NULL)
            {
                for(int j=i; j>=0; j--)
                {
                    free((*bitcode)[j]);
                    (*bitcode)[j]=NULL;
                }
                free(*bitcode);
                *bitcode=NULL;
                return QH_FAILED;
            }
            bzero((*bitcode)[i], sizeof(char)*32);
        }
    }
    else
    {
        get_num=1;
        *num = 0;
    }
    
    if ((dir=opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        return QH_FAILED;
    }
    
    int counter = 0;
    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0 || strcmp(ptr->d_name,"Makefile")==0 )    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == DT_REG)    ///file
        {
            if(!get_num)
            {
                snprintf((*bitcode)[counter], 31*sizeof(char), "%s", ptr->d_name);  //add .bc suffix
                counter++;
//                printf("%d\n", counter);
            }
            else
            {
                *num=*num+1;
            }
            //printf("d_name:%s/%s\n",basePath,ptr->d_name);
        }
        else
            continue;
    }
    
    closedir(dir);
    return 1;
}

ERRORCODE constructMakefile(int index)
{
    ERRORCODE err_code = QH_SUCCESS;
//    static char makefile_template[] = "./Makefile.template";//"/Users/axis/Library/Developer/Xcode/DerivedData/StepMacho-adzvvnpkwtqdgqfolcyxhobkxcfq/Build/Products/Debug/Makefile.template";
    
    //0. construct the path
//    char *makefile_all_path;
//    char makefile_tpl_fmt[] = "%s/%s";
//    char buf[1024];
//    bzero(buf, sizeof(buf));
//    getcwd(buf,sizeof(buf));
//    
//    makefile_all_path = (char*)malloc(sizeof(char)*(sizeof(makefile_tpl_fmt)+1+strlen(makefile_template)+strlen("/")+strlen(buf)));
//    sprintf(makefile_all_path, "%s/%s", buf, makefile_template);
    
    //1. check whether the makefile is existed or not
    if(access(make_file_template, F_OK)!=0)
    {
        return QH_MACHO_NOT_EXISTED;
    }
    
    //2. get arch
    char arch[100];
    bzero(arch, sizeof(char)*100);
    strncpy(arch, archs_sub_str[index], strlen(archs_sub_str[index]));
    
    do
    {
        //3. get bitcode file array
        char **bitcode=NULL;
        int num=0;
        readFileList(info.bitcode_path[index], NULL, &num);         //get the num
        readFileList(info.bitcode_path[index], &bitcode, &num);     //get the file name list
        
        //4. cp make file to the directory
        char cp_cmd_fmt[] = "sudo cp -Rf %s %s/Makefile";
        char cp_cmd[sizeof(cp_cmd_fmt)+strlen(make_file_template)+strlen(info.bitcode_path[index])+1];
        bzero(cp_cmd, sizeof(cp_cmd));
        snprintf(cp_cmd, sizeof(cp_cmd), cp_cmd_fmt, make_file_template, info.bitcode_path[index]);
        executeSystem(cp_cmd);
        
        //5. set arch to the makefile (after try this, i want to set enviroment for repalcing that code)
        char makefile_path_fmt[] = "%s/Makefile";
        char makefile_path[sizeof(makefile_path_fmt)+1+strlen(info.bitcode_path[index])];
        bzero(makefile_path, sizeof(makefile_path));
        snprintf(makefile_path, sizeof(makefile_path), makefile_path_fmt, info.bitcode_path[index]);
//
//        fp = fopen(makefile_path, "rw+");
//        long file_size=-1;
//        if(fp==NULL)
//        {
//            err_code = QH_FAILED;
//            break;
//        }
//        
//        fseek(fp, 0L, SEEK_END);
//        file_size = ftell(fp);
//        
//        int fd = -1;
//        char file_fmt[sizeof(char)*file_size+1];
//        char file_content[sizeof(char)*file_size+strlen(arch)+1];
//        bzero(file_content, sizeof(file_content));
//        bzero(file_fmt, sizeof(file_fmt));
//        
//        fd = fileno(fp);
//        int status = fseek(fp, 0L, SEEK_SET);
//        fread(file_fmt, sizeof(file_fmt)-1, 1, fp);   //read all files
//        snprintf(file_content, sizeof(file_content)-1, file_fmt, arch); // change the file content
//        fwrite(file_content, strlen(file_content), 1, fp);
        //ftruncate(fd, strlen(file_content));    //delete the rest of file
        
        //5.1  make file to .bc
        for(int i=0; i<num; i++)
        {
            if(strlen(bitcode[i])>strlen(".bc"))
            {
                long skip = strlen(bitcode[i])-strlen(".bc");
                if(strncmp(bitcode[i]+skip, ".bc", strlen(".bc"))==0)
                    continue;
            }
            else
            {
                //continue;
            }
            
            char oldname[strlen(bitcode[i])+strlen(info.bitcode_path[index])+10];
            char newname[strlen(bitcode[i])+strlen(info.bitcode_path[index])+strlen(".bc")+10];
            bzero(oldname, sizeof(oldname));
            bzero(newname, sizeof(newname));
            sprintf(oldname, "%s/%s", info.bitcode_path[index], bitcode[i]);
            sprintf(newname, "%s/%s.%s", info.bitcode_path[index], bitcode[i], "bc");
            rename(oldname, newname);
        }
        
        //6. set compile rule
        char ouput_file_rule_fmt[] = "\n\t@$(COMPILE_iOS_BC_2_OBJ)   -c $(ROOT)/%s.bc -o $(ROOT)/%s.o";
        int fd = -1;
        fd = open(makefile_path, O_WRONLY);
        lseek(fd, 0L, SEEK_END);    //move the eof
        
        //7. *.bc-->*.o
        for(int i = 0; i<num; i++)
        {
            char rule[sizeof(ouput_file_rule_fmt)+strlen(bitcode[i])+1];
            bzero(rule, sizeof(rule));
            snprintf(rule, sizeof(rule)-1, ouput_file_rule_fmt, bitcode[i], bitcode[i]);
            write(fd, rule, strlen(rule));
        }
        
        //8. *.o---->macho
        char o_file_fmt[] = " $(ROOT)/%s.o ";
        char compile_macho_fmt[] = "\n\t$(LINK_iOS_BIN)   $(LDFLAGS_CUSTOM) -Wl,-sectcreate,__LLVM,__bundle,%s %s -o $(ROOT)/%s";
        long size = 0;
        for(int i=0; i<num; i++)
        {
            size += strlen(bitcode[i])+sizeof(o_file_fmt);
        }
        size += 1;
        
        char file_list[size];
        char *cursor = file_list;
        bzero(file_list, sizeof(file_list));
        for(int i=0; i<num; i++)
        {
            char file[100];
            bzero(file, sizeof(file));
            sprintf(file, o_file_fmt, bitcode[i]);
            strcpy(cursor, file);
            cursor+=strlen(file);
        }
        char compile_final_rule[sizeof(compile_macho_fmt)+strlen(file_list)+strlen(info.macho_name)+1+strlen(info.bitcode_path[index])];
        bzero(compile_final_rule, sizeof(compile_final_rule));
        snprintf(compile_final_rule, sizeof(compile_final_rule), compile_macho_fmt, info.xar_path[index] ,file_list, info.macho_name);
        write(fd, compile_final_rule, strlen(compile_final_rule));
        
        close(fd);
        
        //free bitcode here
        //fixed me:  memory leak
        for(int i=0; i<num; i++)
        {
            free(bitcode[i]);
        }
        free(bitcode);
        bitcode=NULL;
        
        //5. execute the makefile
        char make_fmt[] = "sudo ARCH=%s ROOT=%s make -f %s/Makefile";
        char cmd_make[sizeof(make_fmt)+strlen(info.arch_sub[index])+strlen(info.bitcode_path[index])+strlen(info.bitcode_path[index])+1];
        bzero(cmd_make, sizeof(cmd_make));
        snprintf(cmd_make, sizeof(cmd_make), make_fmt, info.arch_sub[index], info.bitcode_path[index], info.bitcode_path[index]);
//        printf("%s\n\n\n", cmd_make);
        executeSystem(cmd_make);
        //sleep(5);
        
        INFO("compile successfully");
        
        //6. lipo here      (depresed)
        
    }while(0);
    
    //do GC here
    
    return err_code;
}

ERRORCODE compileMacho(int index)
{
    char file_list_fmt[] = "xar -x -f %s -C %s";
    char extra_file_path_fmt[] = "%s/bitcode";
    
    //1. construct the bitcode path
    char extra_file_path[strlen(extra_file_path_fmt)+strlen(info.path[index])+1];
    bzero(extra_file_path, sizeof(extra_file_path));
    snprintf(extra_file_path, sizeof(extra_file_path), extra_file_path_fmt, info.path[index]);
    strncpy(info.bitcode_path[index], extra_file_path, strlen(extra_file_path));
    
    if(access(info.bitcode_path[index], F_OK)==0)
    {
        remove(info.bitcode_path[index]);
    }
    mkdirs(info.bitcode_path[index]);
    
    //2. construct the xar cmd to extra the file
    char cmd[sizeof(file_list_fmt)+strlen(info.xar_path[index])+strlen(info.bitcode_path[index])+1];
    bzero(cmd, sizeof(cmd));
    
    sprintf(cmd, file_list_fmt, info.xar_path[index], info.bitcode_path[index]);
    
    // i do know the status of execute the command, so i will add status for this function
    //3. execute command
    executeSystem(cmd);     //get bitcode
    
    //4. construct the Makefile
    if(constructMakefile(index)!=QH_SUCCESS)
    {
        return QH_FAILED;
    }
    
    
    return 0;
}

ERRORCODE dumpBitCode(int fp, uint64_t offset, uint64_t size, enum sub_archs sub_arch)
{
    static char path[] = "/var/tmp/";
    static char extension[] = ".xar";
    
    //1. save the data
    char file_path[sizeof(path)+strlen(info.macho_name)+strlen(archs_sub_str[sub_arch])+sizeof(extension)+100];
    bzero(file_path, sizeof(file_path));
    strncpy(file_path, path, sizeof(path));
    strncpy((char*)(file_path+strlen(path)), info.macho_name, strlen(info.macho_name));
    strncpy((char*)(file_path+strlen(path)+strlen(info.macho_name)), "/", 1);       //dir
    
    strncpy(info.work_dir, file_path, strlen(file_path));
    
    strncpy((char*)(file_path+strlen(path)+strlen(info.macho_name)+1), archs_sub_str[sub_arch], strlen(archs_sub_str[sub_arch]));
    strncpy((char*)(file_path+strlen(path)+strlen(info.macho_name)+strlen(archs_sub_str[sub_arch])+1), "/", 1);       //dir
    printf("%s\n", file_path);
    if(access(file_path, F_OK)==0)
    {
        WARNING("dir dose not existed");
        
        char cmd_fmt[]="sudo rm -rf %s";
        char cmd_rm[sizeof(cmd_fmt)+strlen(file_path)+1];
        bzero(cmd_rm, sizeof(cmd_rm));
        sprintf(cmd_rm, cmd_fmt, file_path);
        executeSystem(cmd_rm);
        //remove(file_path);
    }
    mkdirs(file_path);
    strncpy(info.path[info.num_arch], file_path, strlen(file_path));    //get the dir's path
    
    strncpy((char*)(file_path+strlen(path)+strlen(info.macho_name)+2+strlen(archs_sub_str[sub_arch])), archs_sub_str[sub_arch], strlen(archs_sub_str[sub_arch]));
    strncpy((char*)(file_path+strlen(path)+strlen(info.macho_name)+2+strlen(archs_sub_str[sub_arch])+strlen(archs_sub_str[sub_arch])), extension, strlen(extension));
    
    
    strncpy(info.xar_path[info.num_arch], file_path, strlen(file_path));                                //get xar_path
    strncpy(info.arch_sub[info.num_arch], archs_sub_str[sub_arch], sizeof(archs_sub_str[sub_arch]));    //get sub arch

    //2. dump the xar file to the disk
    INFO("[+]prepare to dump the xar file\n");
    int bt_fp = open(file_path, O_WRONLY|O_CREAT);
    char buffer[size];
    READ_FILE_WITH_FIXED(fp, buffer, offset);
    write(bt_fp, buffer, size);
    close(bt_fp);
    INFO("[+]dumping xar file ends\n");
    
    //3. compile
    INFO("[+]prepare to compile new macho\n");
    compileMacho(info.num_arch);
    INFO("[+]compile new macho successfully");
    
    info.num_arch++;
    
    return 0;
}

ERRORCODE HandleMachoSegment(int fp, uint32_t offset)
{
    //1. read macho header here
    // there is a assumption, we read the header using 32 bit.
    char macho_header[sizeof(struct mach_header)];
    READ_FILE_WITH_FIXED(fp, macho_header, offset);
    
    //in macho_header struct, all the memory align is little-endine, i did not swap the uint32_t memory layout.
    struct mach_header *header = (struct mach_header*)macho_header;
    uint32_t magic = header->magic;
    
//    if(capacity>3) return QH_MACHO_UNKNOWN_ARCH;
    
    if(magic==MH_MAGIC_64)
    {
        char macho_header_64[sizeof(struct mach_header_64)];
        READ_FILE_WITH_FIXED(fp, macho_header_64, offset);
     
//#warning complete 64bit ops
        struct mach_header_64 *header_64 __unused = (struct mach_header_64*)macho_header_64;
        
        uint32_t ncmds = header_64->ncmds;
        int is_find = 0;
        uint32_t skip_bytes = offset+sizeof(struct mach_header_64);    //skip the header struct
        char segment_char[sizeof(struct segment_command_64)];
        struct segment_command_64 *seg_64=NULL;
        for(int i=0; i<ncmds; i++)
        {
            READ_FILE_WITH_FIXED(fp, segment_char, skip_bytes);
            
            seg_64 = (struct segment_command_64*)segment_char;
            if(seg_64->cmd==LC_SEGMENT_64)
            {
                if(strcmp(seg_64->segname, LLVM_SEG)==0)
                {
                    is_find = 1;
                    break;
                }
            }
            skip_bytes+=seg_64->cmdsize;
        }
        
        if(is_find)
        {
            //find section information
            if(seg_64->nsects>=1)
            {
                //go through the all sections
                int find_llvm_bundle = 0;
                skip_bytes+=sizeof(struct segment_command_64);
                //for(int i = 0; i<seg_64->nsects; i++)
                struct section_64 *sec=NULL;
                for(int i=0;i<seg_64->nsects;i++)
                {
                    char section_char[sizeof(struct section_64)];
                    READ_FILE_WITH_FIXED(fp, section_char, skip_bytes);
                    sec = (struct section_64*)section_char;
                    if(strncmp(sec->sectname, LLVM_BUNDLE, sizeof(LLVM_BUNDLE))==0)
                    {
                        find_llvm_bundle=1;
                        break;
                    }
                    skip_bytes+=sizeof(struct section_64);
                }
                
                if(find_llvm_bundle==1)
                {
                    dumpBitCode(fp, sec->offset+offset, sec->size, getSubArchs(header_64->cputype, header_64->cpusubtype));
                    return QH_SUCCESS;
                }
                return QH_MACHO_LLVM_ERROR;
            }
            
            return QH_MACHO_LLVM_NO_SECT;
        }
        return QH_MACHO_NOT_CONTAIN_SEG;

    }
    else
    {
        uint32_t ncmds = header->ncmds;
//        printf("cpu=%x", header->cputype);
        int is_find = 0;
        uint32_t skip_bytes = offset+sizeof(struct mach_header);    //skip the header struct
        char segment_char[sizeof(struct segment_command)];
        struct segment_command *seg=NULL;
        for(int i=0; i<ncmds; i++)
        {
            READ_FILE_WITH_FIXED(fp, segment_char, skip_bytes);
            
            seg = (struct segment_command*)segment_char;
            if(seg->cmd==LC_SEGMENT)
            {
                if(strcmp(seg->segname, LLVM_SEG)==0)
                {
                    is_find = 1;
                    break;
                }
            }
            skip_bytes+=seg->cmdsize;
        }
        
        if(is_find)
        {
            //find section information
            if(seg->nsects>=1)
            {
                //go through the all sections
                int find_llvm_bundle = 0;
                struct section *sec=NULL;
                skip_bytes+=sizeof(struct segment_command);
                for(int i = 0; i<seg->nsects; i++)
                {
                    char section_char[sizeof(struct section)];
                    READ_FILE_WITH_FIXED(fp, section_char, skip_bytes);
                    sec = (struct section*)section_char;
                    if(strncmp(sec->sectname, LLVM_BUNDLE, sizeof(LLVM_BUNDLE))==0)
                    {
                        find_llvm_bundle=1;
                        break;
                    }
                    skip_bytes+=sizeof(struct section);
                }
                
                if(find_llvm_bundle==1)
                {
                    dumpBitCode(fp, sec->offset+offset, sec->size, getSubArchs(header->cputype, header->cpusubtype));
                    return QH_SUCCESS;
                }
                return QH_MACHO_LLVM_ERROR;
            }
            
            return QH_MACHO_LLVM_NO_SECT;
        }
        return QH_MACHO_NOT_CONTAIN_SEG;
    }
    
    return -1;
}


ERRORCODE ParseMachoFile(char* macho_path)
{
    ERRORCODE error_code = QH_SUCCESS;
    char buf[1024];
    
    //1. At first, open the macho file
    int fp = open(macho_path, O_RDONLY);
    if(fp<-1) return QH_MACHO_FILE_OPEN_ERROR;  //quit if the open file error
    
    //2. read the 1024 bytes of file
    ssize_t ret = read(fp, buf, sizeof(buf));
    if(ret<0)
    {
        close(fp);
        return QH_MACHO_FILE_READ_ERROR;
    }
    
    //3. check whether the magic
    uint32_t magic = *(uint32_t*)buf;
    if(magic==FAT_CIGAM)    //fat macho
    {
        struct fat_header * fat_header = (struct fat_header *)buf;
        uint32_t narch = LITTLE_ENDINE_32(fat_header->nfat_arch);
        uint64_t offset_byte=sizeof(struct fat_header);
        //4. step over the all archs to find the llvm ir section
        for(uint32_t i = 0; i<narch; i++)
        {
            char tmp_buf[sizeof(struct fat_arch)];
            READ_FILE_WITH_FIXED(fp, tmp_buf, offset_byte);
            
            struct fat_arch * arch = (struct fat_arch *)tmp_buf;       //get the fat_arch pointer
            error_code = HandleMachoSegment(fp, LITTLE_ENDINE_32(arch->offset));
            if(QH_ISERROR(error_code)) return error_code;              // if error, we just quit now
            
            offset_byte+=sizeof(struct fat_arch);   //step another arch
//#warning  add handle ops here
        }
    }
    else                    //maybe non-fat macho
    {
#warning the unit test dose not cover this branch, so we need a new ipa to test the branch
        //4. handle the macho with only one arch
        error_code = HandleMachoSegment(fp, 0); //the arch just start from the very beginning
    }
    
    if(fp>0) close(fp);
    
    //5.lipo thoes macho
    int length = 0;
    char *param=NULL, *pointer;
    for(int i=0; i<info.num_arch; i++)
    {
        length += strlen(info.bitcode_path[i])+strlen("/")+strlen(info.macho_name)+2;
    }
    
    param = (char*)malloc(sizeof(char)*(length+1));
    pointer=param;
    bzero(param, sizeof(char)*(length+1));
    for(int i=0; i<info.num_arch; i++)
    {
        strncpy(pointer, info.bitcode_path[i], strlen(info.bitcode_path[i]));       // dir
        pointer+=strlen(info.bitcode_path[i]);
        strncpy(pointer, "/", 1);                                                   // '/'
        pointer+=1;
        strncpy(pointer, info.macho_name, strlen(info.macho_name));                 // 'macho-name'
        pointer+=strlen(info.macho_name);
        strncpy(pointer, " ", 1);
        pointer+=1;
    }
    
    //printf("%s", param);
    char lipo_cmd_fmt[] = "sudo lipo -create %s -o %s/%s";
    char lipo_cmd[strlen(lipo_cmd_fmt)+strlen(param)+strlen(info.macho_name)+strlen(info.work_dir)+1];
    
    bzero(lipo_cmd, sizeof(lipo_cmd));
    snprintf(lipo_cmd, sizeof(lipo_cmd), lipo_cmd_fmt, param, info.work_dir, info.macho_name);
    executeSystem(lipo_cmd);
    free(param);
    
    //6. mv the new macho to the older one
    INFO("prepare to copy the file");
    char mv_cmd_fmt[] = "sudo cp -Rf %s/%s %s";
    char mv_cmd[sizeof(mv_cmd_fmt)+strlen(macho_path)+strlen(info.work_dir)+strlen(info.macho_name)+1];
    bzero(mv_cmd, sizeof(mv_cmd));
    snprintf(mv_cmd, sizeof(mv_cmd),mv_cmd_fmt, info.work_dir, info.macho_name, macho_path);
    executeSystem(mv_cmd);
    INFO("copy file finished");
    
    sleep(2);
    
//    printf("[cp cmd]%s", mv_cmd);
    
    return error_code;
}

ERRORCODE DumpMachoLLVMSec(const char* macho_path, const char * filename, const char* make_tpl_path)
{
    ERRORCODE error_code = QH_SUCCESS;
    
    bzero(&info, sizeof(info));
    strncpy(info.macho_name, filename, strlen(filename));
    
//    make_file_template=(char*)make_tpl_path;
    make_file_template = (char*)malloc(sizeof(char)*(strlen(make_tpl_path)+1));
    bzero(make_file_template, sizeof(char)*(strlen(make_tpl_path)+1));
    strncpy(make_file_template, make_tpl_path, strlen(make_tpl_path));
    
    do
    {
        //0. try to find whether the file is existed
        if(macho_path==NULL) SET_ERRCODE_WITH_BREAK(error_code, QH_MACHO_NOT_PARM_INVALID);
        if(access(macho_path, F_OK)!=0) SET_ERRCODE_WITH_BREAK(error_code, QH_MACHO_NOT_EXISTED);
        
        //1. try to parse the macho file
        error_code = ParseMachoFile((char*)macho_path);
        if(QH_ISERROR(error_code)) break;
        
    }while(0);
                                       
    if(make_file_template) free(make_file_template);
    
    return error_code;
}
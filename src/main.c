#include "info.h"
#include "logos.h"
#include "stdlib.h"

/* TODO:
 * use strlen() to determine how far to --align ("%-%ds", max(strlen(a), strlen(b)) + 2)
 * option to choose what order the infos are printed in ( modules {"a", "b"} in albafetch.conf)
 * --ascii for custom ascii art (conflicts with --logo)
 * --conf for custom config file (needs to somehow move the argv check before the conf check - even tho argv is meant to override it)
 * remove the lspci dependency for gpu()
 */

// CONFIGURATION OPTIONS:
char *separator_string; // what is used as separator between sections
char *dash; // default separator

// default config
Config config = {
    "\e[0m------------------",          // separator
    "",                                 // separator2
    ":",                                // dash
    true,                               // print_cpu_freq
    true,                               // print_cpu_brand
    true,                               // print_gpu_arch
    true,                               // print_mem_perc
    true,                               // align stats
    "",                                 // color
    "\e[1m",                            // bold
    // Labels:
    "",                                 // title_prefix
    "",                                 // col_prefix
    "███",                              // col_block
    "Hostname",                         // hostname
    "User",                             // user
    "Uptime",                           // uptime
    "OS",                               // os
    "Kernel",                           // kernel
    "Desktop",                          // desktop
    "Shell",                            // shell
    "Shell"                             // default_shell_label
    "Terminal",                         // terminal
    "Packages",                         // packages
    "Host",                             // host
    "BIOS",                             // bios
    "CPU",                              // cpu
    "GPU",                              // gpu
    "Memory",                           // memory
    "Publ. IP",                         // public IP
    "Loc. IP",                          // local IP
    "Directory"                         // pwd
};

bool print_cpu_freq;

bool default_bold = false;
char default_color[33] = "";
char default_logo[33] = "";

char **logo;

char spacing[32] = "    ";
char spacing_first[32] = "";
char spacing_last[32] = "";

void unescape(char *str) {
    // check every \ in the given string and unescape \n and \e
    while((str=strchr(str, '\\'))) {
        memmove(str, str+1, strlen(str+1)+1);
        switch(*str) {
            default: case '\\':
                break;
            case 'n':
                *str = '\n';
                break;
            case 'e':
                *str = '\e';
                break;
        }
    ++str;
    }
}

bool parse_config_option(char* source, char *dest, char *field) {
    char *ptr;

    // looks for the keyword, checks the string between the following " "

    if((ptr = strstr(source, field))) {
        if((ptr = strchr(ptr, '"'))) {
            ++ptr;

            if(((field = strchr(ptr, '"'))) && ptr) {
                *field = 0;
                strcpy(dest, ptr);
                *field = '"';
                return 1;
            }
        }
    }

    return 0;
}

bool parse_config_bool(char *source, bool *dest, char *field) {
    char *ptr;

    // looks for the keyword, checks the string between the following " "

    if((ptr = strstr(source, field))) {
        if((ptr = strchr(ptr, '"'))) {
            ++ptr; 

            if(((field = strchr(ptr, '"'))) && ptr) {
                *field = 0;
                *dest = !strcmp(ptr, "true");
                *field = '"';
                return 1;
            }
        }
    }

    return 0;
}

void parse_config() {
    // really bad code here, you don't need to look

    char path[LOGIN_NAME_MAX + 33];
    snprintf(path, LOGIN_NAME_MAX + 32, "%s/.config/albafetch.conf", getenv("HOME"));

    FILE *fp = fopen(path, "r");
    if(!fp) {
        return;
    }
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    rewind(fp);
    char *conf = malloc(len+1);
    conf[fread(conf, 1, len, fp)] = 0;
    fclose(fp);

    unescape(conf);

    // remove comments
    char *ptr = conf;
    char *ptr2 = conf;
    while((ptr = strchr(ptr, ';'))) {
        ptr2 = strchr(ptr, '\n');
        memmove(ptr, ptr2+1, strlen(ptr2+1)+1);
    }
    ptr = ptr2 = conf;
    while((ptr = strchr(ptr, '#'))) {
        ptr2 = strchr(ptr, '\n');
        memmove(ptr, ptr2+1, strlen(ptr2+1)+1);
    }
    
    // spacing
    parse_config_option(conf, spacing, "spacing");

    // spacing_first
    parse_config_option(conf, spacing_first, "spacing_first");

    // spacing_last
    parse_config_option(conf, spacing_last, "spacing_last");


    // separators
    parse_config_option(conf, config.separator, "separator");
    parse_config_option(conf, config.separator2, "separator2");

    // dash
    parse_config_option(conf, config.dash, "dash");

    // print_cpu_freq
    parse_config_bool(conf, &config.print_cpu_freq, "print_cpu_freq");

    // print_cpu_brand
    parse_config_bool(conf, &config.print_cpu_brand, "print_cpu_brand");

    // print_gpu_arch
    parse_config_bool(conf, &config.print_gpu_arch, "print_gpu_arch");

    // print_mem_perc
    parse_config_bool(conf, &config.print_mem_perc, "print_mem_perc");

    // align_infos
    parse_config_bool(conf, &config.align_infos, "align_infos");
    
    // color
    if(parse_config_option(conf, config.color, "default_color"))
        strcpy(default_color, config.color);

    // bold
    bool bold;
    if((default_bold = parse_config_bool(conf, &bold, "default_bold"))) {
        if(bold)
            strcpy(config.bold, "\e[1m");
        else
            strcpy(config.bold, "");
    }

    // logo
    if((ptr = strstr(conf, "default_logo"))) {
        if((ptr = strchr(ptr, '"'))) {
            ++ptr;
            if(((ptr2 = strchr(ptr, '"'))) && ptr) {
                *ptr2 = 0;
                for(int i = 0; i < sizeof(logos)/sizeof(*logos); ++i)
                    if(!strcmp(logos[i][0], ptr)) {
                        logo = (char**)logos[i];
                        strcpy(default_logo, *logo);
                    }
                *ptr2 = '"';
            }
        }
    }

    // col_block
    if((ptr = strstr(conf, "col_block"))) {
        if((ptr = strchr(ptr, '"'))) {
            ++ptr;
            ptr2 = strchr(ptr, '"');
            *ptr2 = 0;
            strcpy(config.col_block, ptr);
            *ptr2 = '"';
        }
    }

    // LABELS
    // title
    parse_config_option(conf, config.title_prefix, "title_prefix");

    // colors
    parse_config_option(conf, config.col_prefix, "col_prefix");

    // hostname
    parse_config_option(conf, config.hostname_label, "hostname_label");

    // user
    parse_config_option(conf, config.user_label, "user_label");

    // uptime
    parse_config_option(conf, config.uptime_label, "uptime_label");

    // os
    parse_config_option(conf, config.os_label, "os_label");

    // kernel
    parse_config_option(conf, config.kernel_label, "kernel_label");

    // desktop
    parse_config_option(conf, config.desktop_label, "desktop_label");

    // shell (current)
    parse_config_option(conf, config.shell_label, "shell_label");

    // shell(default)
    parse_config_option(conf, config.default_shell_label, "def_shell_label");

    // terminal
    parse_config_option(conf, config.term_label, "term_label");

    // packages
    parse_config_option(conf, config.packages_label, "packages_label");

    // host
    parse_config_option(conf, config.host_label, "host_label");

    // bios
    parse_config_option(conf, config.bios_label, "bios_label");

    // cpu
    parse_config_option(conf, config.cpu_label, "cpu_label");

    // gpu
    parse_config_option(conf, config.gpu_label, "gpu_label");
    
    // memory
    parse_config_option(conf, config.mem_label, "mem_label");

    // public IP
    parse_config_option(conf, config.pub_ip_label, "pub_ip_label");

    // local IP
    parse_config_option(conf, config.loc_ip_label, "loc_ip_label");

    // pwd
    parse_config_option(conf, config.pwd_label, "pwd_label");

    free(conf);
}

int printLogo(const int line) {
    if(*logo[line]) {
        printf("%s%s%s", config.bold, logo[line], config.color);
        return line+1;
    } else {
        printf("%s%s%s", config.bold, logo[2], config.color);
        return line;
    }
}

int main(const int argc, const char **argv) {
    parse_config();
    if(!(*spacing_first)) strcpy(spacing_first, spacing);
    if(!(*spacing_last)) strcpy(spacing_last, spacing);

    bool help = false;
    int line = 3;

    // rtfm and stfu
    bool user_is_an_idiot = false;
    
    // command line arguments
    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            help = 1;
        } else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--color")) {
            if(argv[i+1]) {
                char * colors[9][2] = {
                    {"black", "\e[30m"},
                    {"red", "\e[31m"},
                    {"green", "\e[32m"},
                    {"yellow", "\e[33m"},
                    {"blue", "\e[34m"},
                    {"purple", "\e[35m"},
                    {"cyan", "\e[36m"},
                    {"gray", "\e[90m"},
                    {"white", "\e[37m"},
                };

                for(int j = 0; j < 9; ++j) {
                    if(!strcmp(argv[i+1], *colors[j])) {
                        strcpy(config.color, colors[j][1]);
                        goto color;
                    }
                }

                fputs("\e[31m\e[1mERROR\e[0m: invalid color! Use --help for more info\n", stderr);
                user_is_an_idiot = true;

                color: ;
            } else {
                fputs("\e[31m\e[1mERROR\e[0m: --color requires a color! Use --help for more info\n", stderr);
                user_is_an_idiot = true;
            }
        } else if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bold")) {
            if(argv[i+1]) {
                if(!strcmp(argv[i+1], "on"))
                    strcpy(config.bold, "\e[1m");
                else if(!strcmp(argv[i+1], "off"))
                    strcpy(config.bold, "");
                else {
                    fputs("\e[31m\e[1mERROR\e[0m: invalid value for --bold! Use --help for more info\n", stderr);
                    user_is_an_idiot = true;
                }
            } else {
                fputs("\e[31m\e[1mERROR\e[0m: --bold requires a value! Use --help for more info\n", stderr);
                user_is_an_idiot = true;
            }
        } else if(!strcmp(argv[i], "-l") || !strcmp(argv[i], "--logo")) {
            if(argv[i+1]) {
                for(int j = 0; j < sizeof(logos)/sizeof(*logos); ++j)
                    if(!strcmp(logos[j][0], argv[i+1])) {
                        logo = (char**)logos[j];
                        goto logo_arg_found;
                    }
                logo = (char**)logos[0];
                logo_arg_found: ;
            } else {
                fputs("\e[31m\e[1mERROR\e[0m: --logo requires a value! Use --help for more info\n", stderr);
                user_is_an_idiot = true;
            }
        } else if(!strcmp(argv[i], "--align") || !strcmp(argv[i], "-a")) {
            if(argv[i+1]) {
                if(!strcmp(argv[i+1], "on"))
                    config.align_infos = true;
                else if(!strcmp(argv[i+1], "off"))
                    config.align_infos = false;
                else {
                    fputs("\e[31m\e[1mERROR\e[0m: invalid value for --align! Use --help for more info\n", stderr);
                    user_is_an_idiot = true;
                }
            } else {
                fputs("\e[31m\e[1mERROR\e[0m: --align requires a value! Use --help for more info\n", stderr);
                user_is_an_idiot = true;
            }
        }
    }

    if(user_is_an_idiot)
        return 1;

    // set logo and color if not defined yet
    if(!logo) {
        #ifdef __APPLE__
            logo = (char**)logos[1];
            goto logo_found;
        #else
            FILE *fp = fopen("/etc/os-release", "r");
            if(!fp) {
                logo = (char**)logos[0];
                goto logo_found;
            }

            char os_id[32];
            read_after_sequence(fp, "\nID", os_id, 32);
            if(!(*os_id))
                read_after_sequence(fp, "ID", os_id, 32);
            fclose(fp);

            char *end = strchr(os_id, '\n');
            if(!end)
                return -1;
            *end = 0;

            for(int i = 0; i < sizeof(logos)/sizeof(*logos); ++i)
                if(!strcmp(logos[i][0], os_id)) {
                    logo = (char**)logos[i];
                    goto logo_found;
                }
            logo = (char**)logos[0];

            logo_found: ;
        #endif
    }
    if(!(*config.color))
        strcpy(config.color, logo[1]);

    if(help) {  // print the help message if --help was used and exit
        printf("%s%salbafetch\e[0m - a system fetch utility\n",
               config.color, config.bold);

        printf("\n%s%sFLAGS\e[0m:\n",
               config.color, config.bold);

        printf("\t%s%s-h\e[0m,%s%s --help\e[0m:\t Print this help menu and exit\n",
               config.color, config.bold, config.color, config.bold);

        printf("\t%s%s-c\e[0m,%s%s --color\e[0m:\t Change the output color (%s%s\e[0m)\n"
               "\t\t\t   [\e[30mblack\e[0m, \e[31mred\e[0m, \e[32mgreen\e[0m, \e[33myellow\e[0m,"
               " \e[34mblue\e[0m, \e[35mpurple\e[0m, \e[36mcyan\e[0m, \e[90mgray\e[0m,"
               " \e[37mwhite\e[0m]\n",
               config.color, config.bold, config.color, config.bold, default_color[0] ? default_color : logo[1], default_color[0] ? "default" : "logo default");

        printf("\t%s%s-b\e[0m,%s%s --bold\e[0m:\t Specifies if bold should be used in colored parts (default: %s\e[0m)\n"
               "\t\t\t   [\e[1mon\e[0m, off]\n",
               config.color, config.bold, config.color, config.bold, default_bold ? "\e[1mon" : "off");
        
        printf("\t%s%s-l\e[0m,%s%s --logo\e[0m:\t Changes the logo that will be displayed (%s)\n"
               "\t\t\t   [linux, apple, arch, arch_small, debian, linuxmint, endeavouros, ubuntu]\n"
               "\t\t\t   [parrot, manjaro, fedora, neon, pop, gentoo, windows]\n",
               config.color, config.bold, config.color,config. bold, default_logo[0] ? default_logo : "OS default");

        printf("\t%s%s-a\e[0m, %s%s--align\e[0m:\t Alignes the infos if set (default: %s\e[0m)\n"
               "\t\t\t   [on, off]\n", config.color, config.bold, config.color, config.bold, config.align_infos ? "on" : "off");

        printf("\nReport a bug: %s%shttps://github.com/alba4k/albafetch/issues\e[0m\n",
               config.color, config.bold);

        return 0;
    }

    void (*infos[])() = {
        title,
        separator,
        uptime,
        separator, 
        os,
        kernel,
        desktop, 
        shell,
        term,
        packages, 
        separator,
        host,
        cpu,
        gpu,
        memory,
        separator2,
        colors,
        light_colors
    };

    // The following line works because infos is declared on the stack,
    // so sizeof returns it's real size and not the size of a pointer.
    const int info_lines = (int)(sizeof(infos) / sizeof(*infos)) - 1;
    
    for(int i = 0; i <= info_lines; ++i) {
        line = printLogo(line);
        if(line == 4) printf("%s", spacing_first);
        else if(i == info_lines) printf("%s", spacing_last);
        else printf("%s", spacing);
        infos[i]();
        printf("\n");
    }

    // ******** remaining lines of the logo ********
    while(*logo[line]) {
        line = printLogo(line);
        printf("\n");
    }
    printf("\e[0m");
    fflush(stdout);

    return 0;
}

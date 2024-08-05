#include "info.h"
#include "../utils.h"

#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// get the current Cursor Theme
int cursor_theme(char *dest){ 
    // try using gsettings
    // reading ~/.config/gtk-3.0/settings.ini could also be an option 
    if(access("/bin/gsettings", F_OK) == 0){
        char buf[256] = "";
        char *args[] = {"gsettings", "get", "org.gnome.desktop.interface", "cursor-theme", NULL};
        exec_cmd(buf, 256, args);

        // cleanup
        if(buf[0] != 0) {
            if(buf[0] == '\'') {
                strcpy(buf, buf+1);

                char *ptr = strchr(buf, '\'');
                if(ptr)
                    *ptr = 0;
            }

            strncpy(dest, buf, 256);
            return 0;
        }
    }

    return 1;
}

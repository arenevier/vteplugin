/*
 This program is free software. It comes without any warranty, to
 the extent permitted by applicable law. You can redistribute it
 and/or modify it under the terms of the Do What The Fuck You Want
 To Public License, Version 2, as published by Sam Hocevar. See
 http://sam.zoy.org/wtfpl/COPYING for more details.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2010 arno renevier <arno@renevier.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include "npapi.h"
#include "npruntime.h"

#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <vte/vte.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
#define DEBUG_DEFAULT_FNAME "vteplugin_debug.txt"
FILE* debug_file;
void debug_start() {
    char* filename;
    if (!(filename = strdup(getenv("VTEPLUGIN_DEBUG_FNAME")))) {
        const char* tmpdir = g_get_tmp_dir();
        filename = g_strdup_printf("%s/%s", tmpdir, DEBUG_DEFAULT_FNAME);
    }
    debug_file = fopen(filename, "w");
    free (filename);
}
void debug_stop() {
    fclose (debug_file);
}
void debug(char* fmt, ...) {
    if (!debug_file) {
        debug_start();
    }
    if (!debug_file) {
        return;
    }

    char datestring [60];
    struct tm *tm;
    time_t rawtime;
    rawtime = time (NULL);
    tm = localtime (&rawtime);
    strftime (datestring, 60, "%T", tm);
    fprintf(debug_file, "[%s] ", datestring);

    va_list args;
    va_start (args, fmt);
    vfprintf (debug_file, fmt, args);
    va_end(args);
}
#else
void debug_start() {}
void debug_stop() {}
void debug(char* fmt, ...) {}
#endif

typedef struct _PluginInstance
{
    char *bgcolor;
    char *fgcolor;
    char *font;
    NPWindow *window;
} PluginInstance;

static char* NPStringUTF8ToChar(NPString* string) {
    if (!string) {
        return NULL;
    }

    char *outString = (char *)malloc(string->UTF8Length + 1);
    if (!outString) {
        return NULL;
    }
    strcpy(outString, string->UTF8Characters);
    outString[string->UTF8Length] = '\0';
    return outString;
}

static char* URLProtoForInstance(NPP instance) {
    char* res = NULL;

    if (!instance) {
       return NULL; 
    }

    NPObject* pluginElement = NULL;
    if (NPN_GetValue(instance, NPNVPluginElementNPObject, &pluginElement) == NPERR_NO_ERROR && pluginElement) {
        NPIdentifier documentIdentifier = NPN_GetStringIdentifier("ownerDocument");
        NPVariant documentVariant;
        if (NPN_GetProperty(instance, pluginElement, documentIdentifier, &documentVariant)) {
            NPObject *ownerDocument = NPVARIANT_TO_OBJECT(documentVariant);
            if (ownerDocument) {
                NPIdentifier locationIdentifier = NPN_GetStringIdentifier("location");
                NPVariant locationVariant;
                if (NPN_GetProperty(instance, ownerDocument, locationIdentifier, &locationVariant)) {
                    NPObject *location = NPVARIANT_TO_OBJECT(locationVariant);
                    if (location) {
                        NPIdentifier protoIdentifier = NPN_GetStringIdentifier("protocol");
                        NPVariant protoVariant;
                        if (NPN_GetProperty(instance, location, protoIdentifier, &protoVariant)) {
                            NPString* protoStr = &NPVARIANT_TO_STRING(protoVariant);
                            if (protoStr) {
                                 res = NPStringUTF8ToChar(protoStr);
                            }
                            NPN_ReleaseVariantValue (&protoVariant);
                        }
                    }
                    NPN_ReleaseVariantValue (&locationVariant);
                }
            }
            NPN_ReleaseVariantValue (&documentVariant);
        }
        NPN_ReleaseObject(pluginElement);
    }
    return res;
}

/* 
 * security check: only load plugin if it's called from file:// protocol or
 * possibly chrome:// for gecko browsers
*/
bool checkSecurityInstance(NPP instance) {
    char* proto = URLProtoForInstance (instance);
    if (!proto) {
        return FALSE;
    }

    if (strcmp(proto, "file:") == 0) { // allow from file: protocol
        return TRUE;
    }

    if (strcmp(proto, "chrome:") == 0) {
        const char* useragent = NPN_UserAgent(instance);
        if (strcasestr(useragent, "gecko")) { // allow from chrome: protocol if useragent contains gecko string
            return TRUE;
        }
    }
    return FALSE;
}

char*
NPP_GetMIMEDescription(void)
{
    debug("%s\n", __func__);
    return("application/vte::vte emulator");
}


NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;

    debug("%s\n", __func__);
    switch (variable) {
        case NPPVpluginNameString:
            *((char **)value) = "terminal emulator";
            break;
        case NPPVpluginDescriptionString:
            *((char **)value) = "allow embedding a terminal emulator inside a browser";
            break;
     case NPPVpluginNeedsXEmbed:
          *((NPBool *)value) = 1;
          break; 
        default:
            err = NPERR_GENERIC_ERROR;
    }
    return err;
}

NPError
NPP_Initialize(void)
{
    debug_start();
    debug("%s\n", __func__);
    return NPERR_NO_ERROR;
}

void
NPP_Shutdown(void)
{
    debug("%s\n", __func__);
    debug_stop();
}

NPError 
NPP_New(NPMIMEType pluginType,
    NPP instance,
    uint16_t mode,
    int16_t argc,
    char* argn[],
    char* argv[],
    NPSavedData* saved)
{
    debug("%s\n", __func__);

    if (instance == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    if (!checkSecurityInstance(instance)) {
        return NPERR_INVALID_URL;
    }

    if (pluginType == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }


    instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));

    PluginInstance* This = (PluginInstance*) instance->pdata;

    if (This == NULL) {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }

    memset(This, 0, sizeof(PluginInstance));

    int idx;
    for (idx = 0; idx < argc; idx++) {
        if (strcasecmp("bgcolor", argn[idx]) == 0) {
            This->bgcolor = strdup(argv[idx]);
        } else if (strcasecmp("fgcolor", argn[idx]) == 0) {
            This->fgcolor = strdup(argv[idx]);
        } else if (strcasecmp("font", argn[idx]) == 0) {
            This->font = strdup(argv[idx]);
        }
    }

    return NPERR_NO_ERROR;
}

NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
    debug("%s\n", __func__);

    if (instance == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    PluginInstance* This = (PluginInstance*) instance->pdata;
    if (This != NULL) {
        if (This->bgcolor) {
            free (This->bgcolor);
        }
        if (This->fgcolor) {
            free (This->fgcolor);
        }
        if (This->font) {
            free (This->font);
        }
        NPN_MemFree(instance->pdata);
        instance->pdata = NULL;
    }

     return NPERR_NO_ERROR;
}

NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
    debug("%s: %d, %d\n", __func__, window->height, window->width);

    if (instance == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    bool xembedSupported = 0;
    NPN_GetValue(instance, NPNVSupportsXEmbedBool, &xembedSupported);
    if (!xembedSupported)
    {
        debug("%s: XEmbed not supported\n", __func__);
        return NPERR_GENERIC_ERROR;
    }

    if (window == NULL) {
        return NPERR_NO_ERROR;
    }

    if (window->window == NULL) {
        return NPERR_NO_ERROR;
    }

    PluginInstance* This = (PluginInstance*) instance->pdata;

    if (This == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    if (This->window) {
        if (This->window != window) {
            debug("%d: This->window (%p) is not equal to window (%p)\n", 
                    __func__, This->window, window);
            return NPERR_INVALID_INSTANCE_ERROR;
        }
        return NPERR_NO_ERROR;
    }

    GtkWidget* plug = gtk_plug_new((GdkNativeWindow)(window->window));
    GtkWidget* terminal = vte_terminal_new();

    //XXX: needs to allocate size here because sometimes webkit will allocate
    //plugin widget size before calling setwindow (ie: while plugin widget does
    //not have child
    GtkAllocation allocation = {window->x, window->y, window->width, window->height};
    gtk_widget_size_allocate (terminal, &allocation);


    // adds VTEPLUGIN environment variable, so it's possible to known if we're
    // inside VTEPLUGIN
    char* env[2];
    env[0] = "VTEPLUGIN=1";
    env[1] = NULL;

    vte_terminal_set_default_colors(VTE_TERMINAL(terminal));
    if (This->bgcolor) {
        GdkColor bgcolor ;
        if (gdk_color_parse (This->bgcolor, &bgcolor)) {
            vte_terminal_set_color_background (VTE_TERMINAL (terminal), &bgcolor);
       }
    }

    if (This->fgcolor) {
        GdkColor fgcolor ;
        if (gdk_color_parse (This->fgcolor, &fgcolor)) {
            vte_terminal_set_color_foreground (VTE_TERMINAL (terminal), &fgcolor);
        }
    }

    if (This->font) {
        vte_terminal_set_font_from_string (VTE_TERMINAL(terminal), This->font);
    }

    vte_terminal_fork_command(VTE_TERMINAL(terminal), NULL, NULL, env, NULL, TRUE, TRUE, TRUE);
    gtk_container_add(GTK_CONTAINER(plug), terminal);
    gtk_widget_show_all(plug);

    This->window = window;

    return NPERR_NO_ERROR;
}

NPError 
NPP_NewStream(NPP instance,
          NPMIMEType type,
          NPStream *stream, 
          NPBool seekable,
          uint16_t *stype)
{
    debug("%s\n", __func__);
    if (instance == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    return NPERR_NO_ERROR;
}

int32_t 
NPP_WriteReady(NPP instance, NPStream *stream)
{
    debug("%s\n", __func__);
    if (instance == NULL) {
        return -1;
    }

    /* We don't want any data, kill the stream */
    //NPN_DestroyStream(instance, stream, NPRES_DONE);

    /* Number of bytes ready to accept in NPP_Write() */
//    return -1;   /* don't accept any bytes in NPP_Write() */
      return 1024;   /* destroying stream before delivering all data results in document load error in webkit */
}

int32_t 
NPP_Write(NPP instance, NPStream *stream, int32_t offset, int32_t len, void *buffer)
{
    debug("%s\n", __func__);
    if (instance == NULL) {
        return -1;
    }

    /* We don't want any data, kill the stream */
    //NPN_DestroyStream(instance, stream, NPRES_DONE);

    //return -1;   /* don't accept any bytes in NPP_Write() */
    return len;   /* destroying stream before delivering all data results in document load error in webkit */
}

NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
    debug("%s\n", __func__);
    if (instance == NULL) {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    return NPERR_NO_ERROR;
}

void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
    debug("%s\n", __func__);
}

void
NPP_URLNotify(NPP instance, const char* url,
                NPReason reason, void* notifyData)
{
    debug("%s\n", __func__);
}

void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
    debug("%s\n", __func__);
}

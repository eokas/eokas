#include "../NativeDialogs.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "NativeInternal.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace eokas
{
    namespace
    {
        bool ensureGtk()
        {
            static std::once_flag once;
            static bool ready = false;
            std::call_once(once, [] {
                ensureXThreads();
                ready = gtk_init_check(nullptr, nullptr);
            });
            return ready;
        }

        const char* utf8OrEmpty(const String& text)
        {
            const char* utf8 = text.cstr();
            return utf8 != nullptr ? utf8 : "";
        }

        void attachOwner(GtkWidget* dialog, void* owner)
        {
            if (dialog == nullptr || owner == nullptr) {
                return;
            }
            gtk_widget_realize(dialog);
            GdkWindow* gdkWindow = gtk_widget_get_window(dialog);
            if (gdkWindow == nullptr) {
                return;
            }
            Display* display = gdk_x11_display_get_xdisplay(gdk_window_get_display(gdkWindow));
            if (display == nullptr) {
                return;
            }
            Window xid = gdk_x11_window_get_xid(gdkWindow);
            Window ownerWindow = static_cast<Window>(reinterpret_cast<uintptr_t>(owner));
            if (xid == None || ownerWindow == None) {
                return;
            }

            XErrorHandler previous = XSetErrorHandler(&ignoreXError);
            XWindowAttributes attributes = {};
            Status status = XGetWindowAttributes(display, ownerWindow, &attributes);
            if (status != 0) {
                XSetTransientForHint(display, xid, ownerWindow);
            }
            XSync(display, False);
            XSetErrorHandler(previous);
        }

        int runDialog(GtkWidget* dialog, void* owner)
        {
            gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
            attachOwner(dialog, owner);
            return gtk_dialog_run(GTK_DIALOG(dialog));
        }

        void closeDialog(GtkWidget* dialog)
        {
            gtk_widget_destroy(dialog);
            while (gtk_events_pending()) {
                gtk_main_iteration_do(FALSE);
            }
        }

        String patternFor(const String& token)
        {
            if (token.contains("*")) {
                return token;
            }
            if (token.startsWith(".")) {
                return String("*") + token;
            }
            return String("*.") + token;
        }

        void addFilters(GtkWidget* dialog, const std::map<String, String>& filters)
        {
            for (const auto& entry : filters) {
                std::vector<std::string> patterns;
                StringVector parts = entry.second.split(";");
                for (const String& part : parts) {
                    String token = part.trim();
                    if (token.isEmpty()) {
                        continue;
                    }
                    patterns.push_back(patternFor(token).cstr());
                }
                if (patterns.empty()) {
                    continue;
                }

                GtkFileFilter* filter = gtk_file_filter_new();
                const char* name = entry.first.cstr();
                gtk_file_filter_set_name(filter, name != nullptr && name[0] != '\0' ? name : "Files");
                for (const std::string& pattern : patterns) {
                    gtk_file_filter_add_pattern(filter, pattern.c_str());
                }
                gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
            }
        }

        bool chosenPath(GtkWidget* dialog, String& selectedPath)
        {
            GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
            if (file == nullptr) {
                return false;
            }
            char* path = g_file_get_path(file);
            g_object_unref(file);
            if (path == nullptr) {
                return false;
            }
            selectedPath = String(path);
            g_free(path);
            return !selectedPath.isEmpty();
        }
    }

    void NativeDialogs::alert(const String& title, const String& message, void* owner)
    {
        if (!ensureGtk()) {
            return;
        }
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "%s",
            utf8OrEmpty(message));
        gtk_window_set_title(GTK_WINDOW(dialog), utf8OrEmpty(title));
        runDialog(dialog, owner);
        closeDialog(dialog);
    }

    int NativeDialogs::confirm(const String& title, const String& message, void* owner)
    {
        if (!ensureGtk()) {
            return 0;
        }
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            "%s",
            utf8OrEmpty(message));
        gtk_window_set_title(GTK_WINDOW(dialog), utf8OrEmpty(title));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Yes", 1);
        gtk_dialog_add_button(GTK_DIALOG(dialog), "No", -1);
        gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel", 0);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), 1);
        int response = runDialog(dialog, owner);
        closeDialog(dialog);
        if (response == 1) return 1;
        if (response == -1) return -1;
        return 0;
    }

    bool NativeDialogs::openFileDialog(String& selectedPath, const std::map<String, String>& filters, void* owner)
    {
        if (!ensureGtk()) {
            return false;
        }
        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            "Open",
            nullptr,
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Ok", GTK_RESPONSE_ACCEPT,
            nullptr);
        gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);
        addFilters(dialog, filters);
        int response = runDialog(dialog, owner);
        bool selected = response == GTK_RESPONSE_ACCEPT && chosenPath(dialog, selectedPath);
        closeDialog(dialog);
        return selected;
    }

    bool NativeDialogs::openFolderDialog(String& selectedPath, const String& defaultPath, void* owner)
    {
        if (!ensureGtk()) {
            return false;
        }
        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            "Open",
            nullptr,
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Ok", GTK_RESPONSE_ACCEPT,
            nullptr);
        GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
        gtk_file_chooser_set_local_only(chooser, TRUE);
        gtk_file_chooser_set_select_multiple(chooser, FALSE);
        if (!defaultPath.isEmpty()) {
            GFile* folder = g_file_new_for_path(utf8OrEmpty(defaultPath));
            gtk_file_chooser_set_current_folder_file(chooser, folder, nullptr);
            g_object_unref(folder);
        }
        int response = runDialog(dialog, owner);
        bool selected = response == GTK_RESPONSE_ACCEPT && chosenPath(dialog, selectedPath);
        closeDialog(dialog);
        return selected;
    }
}

// gcc -o calendar_win32.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")

#define MAX_DESC 200
#define MAX_LOC 100
#define DATA_FILE "calendar.dat"

// Control IDs
#define ID_CALENDAR 1001
#define ID_ADD_BTN 1002
#define ID_DELETE_BTN 1003
#define ID_EDIT_BTN 1004
#define ID_LIST 1005
#define ID_VIEW_ALL 1006
#define ID_VIEW_TODAY 1007
#define ID_EXPORT 1008
#define ID_STATS 1009
#define ID_SEARCH 1010
#define ID_SEARCH_BOX 1011
#define ID_FILTER_CAT 1012
#define ID_FILTER_PRI 1013
#define ID_VIEW_DETAILS 1014
#define ID_IMPORT 1015
#define ID_BACKUP 1016

// Dialog controls
#define IDC_DESC 2001
#define IDC_LOC 2002
#define IDC_HOUR 2003
#define IDC_MIN 2004
#define IDC_END_HOUR 2005
#define IDC_END_MIN 2006
#define IDC_PRIORITY 2007
#define IDC_CATEGORY 2008
#define IDC_ALL_DAY 2009
#define IDC_REMINDER 2011
#define IDC_REMINDER_MIN 2012
#define IDC_SAVE 2013
#define IDC_CANCEL 2014

// Keyboard shortcuts
#define IDM_NEW 3001
#define IDM_EDIT 3002
#define IDM_DELETE 3003
#define IDM_SEARCH 3004
#define IDM_REFRESH 3005

typedef enum {
    PRIORITY_LOW = 0, PRIORITY_MEDIUM, PRIORITY_HIGH, PRIORITY_CRITICAL
} Priority;

typedef enum {
    CAT_WORK, CAT_PERSONAL, CAT_BIRTHDAY, CAT_MEETING,
    CAT_APPOINTMENT, CAT_REMINDER, CAT_HOLIDAY, CAT_OTHER
} Category;


typedef struct {
    int day, month, year;
} Date;

typedef struct {
    int hour, minute;
} Time;

typedef struct Event {
    int id;
    Date date;
    Time start_time, end_time;
    char description[MAX_DESC];
    char location[MAX_LOC];
    Priority priority;
    Category category;
    int is_all_day;
    int reminder_minutes;
    int deleted;
    struct Event *next;
} Event;

// Global variables
Event *event_list = NULL;
int next_id = 1;
HWND hwndMain, hwndCalendar, hwndListView, hwndStatus, hwndSearchBox;
HWND hwndAddDialog = NULL;
HINSTANCE hInst;
Date g_selected_date;
int g_edit_mode = 0;
int g_edit_event_id = 0;

// Filter state
char g_search_filter[MAX_DESC] = "";
int g_category_filter = -1; // -1 = all
int g_priority_filter = -1; // -1 = all

// Utility functions
int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int days_in_month(int month, int year) {
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && is_leap_year(year)) return 29;
    return days[month-1];
}

int compare_dates(Date d1, Date d2) {
    if (d1.year != d2.year) return d1.year - d2.year;
    if (d1.month != d2.month) return d1.month - d2.month;
    return d1.day - d2.day;
}

void get_today(Date *d) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    d->day = st.wDay;
    d->month = st.wMonth;
    d->year = st.wYear;
}

const char* priority_to_string(Priority p) {
    switch(p) {
        case PRIORITY_LOW: return "Low";
        case PRIORITY_MEDIUM: return "Medium";
        case PRIORITY_HIGH: return "High";
        case PRIORITY_CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

const char* category_to_string(Category c) {
    const char *cats[] = {"Work", "Personal", "Birthday", "Meeting",
                          "Appointment", "Reminder", "Holiday", "Other"};
    return cats[c];
}


COLORREF get_priority_color(Priority p) {
    switch(p) {
        case PRIORITY_CRITICAL: return RGB(255, 200, 200);
        case PRIORITY_HIGH: return RGB(255, 230, 200);
        case PRIORITY_MEDIUM: return RGB(255, 255, 200);
        case PRIORITY_LOW: return RGB(200, 255, 200);
        default: return RGB(255, 255, 255);
    }
}

COLORREF get_category_color(Category c) {
    switch(c) {
        case CAT_WORK: return RGB(200, 220, 255);
        case CAT_PERSONAL: return RGB(220, 255, 220);
        case CAT_BIRTHDAY: return RGB(255, 200, 255);
        case CAT_MEETING: return RGB(255, 240, 200);
        case CAT_APPOINTMENT: return RGB(220, 240, 255);
        case CAT_REMINDER: return RGB(255, 255, 220);
        case CAT_HOLIDAY: return RGB(255, 220, 220);
        case CAT_OTHER: return RGB(240, 240, 240);
        default: return RGB(255, 255, 255);
    }
}

// Event management
Event* create_event(Date date, Time start, Time end, const char *desc,
                   const char *loc, Priority pri, Category cat,
                   int all_day, int reminder) {
    Event *e = (Event*)malloc(sizeof(Event));
    if (!e) return NULL;
    
    e->id = next_id++;
    e->date = date;
    e->start_time = start;
    e->end_time = end;
    strncpy(e->description, desc, MAX_DESC-1);
    e->description[MAX_DESC-1] = '\0';
    strncpy(e->location, loc, MAX_LOC-1);
    e->location[MAX_LOC-1] = '\0';
    e->priority = pri;
    e->category = cat;
    e->is_all_day = all_day;
    e->reminder_minutes = reminder;
    e->deleted = 0;
    e->next = NULL;
    
    return e;
}

void add_event_to_list(Event *e) {
    if (!event_list) {
        event_list = e;
        return;
    }
    Event *curr = event_list;
    while (curr->next) curr = curr->next;
    curr->next = e;
}

Event* find_event_by_id(int id) {
    Event *e = event_list;
    while (e) {
        if (e->id == id && !e->deleted) return e;
        e = e->next;
    }
    return NULL;
}

void delete_event(int id) {
    Event *e = find_event_by_id(id);
    if (e) e->deleted = 1;
}

int has_events_on_date(Date d) {
    Event *e = event_list;
    while (e) {
        if (!e->deleted && compare_dates(e->date, d) == 0) {
            return 1;
        }
        e = e->next;
    }
    return 0;
}

// File I/O
void save_events() {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) return;
    
    int magic = 0xCAFEBABE;
    fwrite(&magic, sizeof(int), 1, fp);
    fwrite(&next_id, sizeof(int), 1, fp);
    
    int count = 0;
    Event *e = event_list;
    while (e) {
        if (!e->deleted) count++;
        e = e->next;
    }
    fwrite(&count, sizeof(int), 1, fp);
    
    e = event_list;
    while (e) {
        if (!e->deleted) {
            fwrite(e, sizeof(Event) - sizeof(Event*), 1, fp);
        }
        e = e->next;
    }
    fclose(fp);
}

void load_events() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return;
    
    int magic, count;
    fread(&magic, sizeof(int), 1, fp);
    if (magic != 0xCAFEBABE) {
        fclose(fp);
        return;
    }
    
    fread(&next_id, sizeof(int), 1, fp);
    fread(&count, sizeof(int), 1, fp);
    
    for (int i = 0; i < count; i++) {
        Event *e = (Event*)malloc(sizeof(Event));
        if (!e) break;
        fread(e, sizeof(Event) - sizeof(Event*), 1, fp);
        e->next = NULL;
        add_event_to_list(e);
    }
    fclose(fp);
}

void backup_data() {
    char filename[MAX_PATH];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(filename, "calendar_backup_%04d%02d%02d_%02d%02d%02d.dat",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    
    FILE *src = fopen(DATA_FILE, "rb");
    if (!src) return;
    
    FILE *dst = fopen(filename, "wb");
    if (!dst) {
        fclose(src);
        return;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    
    fclose(src);
    fclose(dst);
    
    char msg[500];
    sprintf(msg, "Backup created successfully:\n%s", filename);
    MessageBox(hwndMain, msg, "Backup Complete", MB_OK | MB_ICONINFORMATION);
}

void export_to_csv(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "ID,Date,Time,Description,Location,Priority,Category,Reminder\n");
    
    Event *e = event_list;
    while (e) {
        if (!e->deleted) {
            fprintf(fp, "%d,%02d/%02d/%d,", e->id, e->date.day, e->date.month, e->date.year);
            if (e->is_all_day) {
                fprintf(fp, "All Day,");
            } else {
                fprintf(fp, "%02d:%02d-%02d:%02d,", 
                       e->start_time.hour, e->start_time.minute,
                       e->end_time.hour, e->end_time.minute);
            }
            fprintf(fp, "\"%s\",\"%s\",%s,%s,%d min\n",
                   e->description, e->location,
                   priority_to_string(e->priority),
                   category_to_string(e->category),
                   e->reminder_minutes);
        }
        e = e->next;
    }
    fclose(fp);
}

// UI Functions
void update_list_view(Date *filter_date) {
    ListView_DeleteAllItems(hwndListView);
    
    Event *e = event_list;
    int idx = 0;
    int total = 0;
    
    while (e) {
        if (!e->deleted) {
            int show = 1;
            
            // Apply date filter
            if (filter_date && compare_dates(e->date, *filter_date) != 0) {
                show = 0;
            }
            
            // Apply search filter
            if (show && strlen(g_search_filter) > 0) {
                char desc_lower[MAX_DESC], search_lower[MAX_DESC];
                strcpy(desc_lower, e->description);
                strcpy(search_lower, g_search_filter);
                _strlwr(desc_lower);
                _strlwr(search_lower);
                if (strstr(desc_lower, search_lower) == NULL) {
                    show = 0;
                }
            }
            
            // Apply category filter
            if (show && g_category_filter != -1 && e->category != g_category_filter) {
                show = 0;
            }
            
            // Apply priority filter
            if (show && g_priority_filter != -1 && e->priority != g_priority_filter) {
                show = 0;
            }
            
            if (show) {
                LVITEM lvi = {0};
                char buffer[50];
                
                // Column 0: ID
                lvi.mask = LVIF_TEXT | LVIF_PARAM;
                lvi.iItem = idx;
                lvi.iSubItem = 0;
                lvi.lParam = (LPARAM)e->id;
                sprintf(buffer, "%d", e->id);
                lvi.pszText = buffer;
                int item_idx = ListView_InsertItem(hwndListView, &lvi);
                
                if (item_idx == -1) {
                    MessageBox(hwndMain, "Failed to insert item!", "Debug", MB_OK);
                    continue;
                }
                
                // Date
                sprintf(buffer, "%02d/%02d/%d", e->date.day, e->date.month, e->date.year);
                ListView_SetItemText(hwndListView, item_idx, 1, buffer);
                
                // Time
                if (e->is_all_day) {
                    strcpy(buffer, "All Day");
                } else {
                    sprintf(buffer, "%02d:%02d-%02d:%02d", 
                           e->start_time.hour, e->start_time.minute,
                           e->end_time.hour, e->end_time.minute);
                }
                ListView_SetItemText(hwndListView, item_idx, 2, buffer);
                
                // Description
                ListView_SetItemText(hwndListView, item_idx, 3, e->description);
                
                // Location
                ListView_SetItemText(hwndListView, item_idx, 4, e->location);
                
                // Priority
                ListView_SetItemText(hwndListView, item_idx, 5, (char*)priority_to_string(e->priority));
                
                // Category
                ListView_SetItemText(hwndListView, item_idx, 6, (char*)category_to_string(e->category));
                
                idx++;
            }
            total++;
        }
        e = e->next;
    }
    
    char status[100];
    sprintf(status, "Total Events: %d | Showing: %d", total, idx);
    SetWindowText(hwndStatus, status);
    
    // Force redraw
    InvalidateRect(hwndListView, NULL, TRUE);
    UpdateWindow(hwndListView);
}

void debug_print_events() {
    int count = 0;
    Event *e = event_list;
    char msg[1000] = "Events in memory:\n\n";
    
    while (e && count < 5) {  // Show first 5 events
        if (!e->deleted) {
            char temp[200];
            sprintf(temp, "ID:%d - %s (%02d/%02d/%d)\n", 
                   e->id, e->description, e->date.day, e->date.month, e->date.year);
            strcat(msg, temp);
            count++;
        }
        e = e->next;
    }
    
    if (count == 0) {
        strcat(msg, "NO EVENTS FOUND!\n");
    }
    
    MessageBox(hwndMain, msg, "Debug: Events", MB_OK);
}

void show_event_details(int event_id) {
    Event *e = find_event_by_id(event_id);
    if (!e) return;
    
    char details[2000];
    char time_str[100];
    
    if (e->is_all_day) {
        strcpy(time_str, "All Day");
    } else {
        sprintf(time_str, "%02d:%02d - %02d:%02d", 
               e->start_time.hour, e->start_time.minute,
               e->end_time.hour, e->end_time.minute);
    }
    
    sprintf(details,
           "===================================\n"
           "           EVENT DETAILS\n"
           "===================================\n\n"
           "ID: %d\n"
           "Description: %s\n"
           "Location: %s\n\n"
           "Date: %02d/%02d/%d\n"
           "Time: %s\n\n"
           "Priority: %s\n"
           "Category: %s\n\n"
           "%s"
           "===================================",
           e->id, e->description, 
           strlen(e->location) > 0 ? e->location : "(No location)",
           e->date.day, e->date.month, e->date.year,
           time_str,
           priority_to_string(e->priority),
           category_to_string(e->category),
           e->reminder_minutes > 0 ? 
               "Reminder: Set\n\n" : "");
    
    MessageBox(hwndMain, details, "Event Details", MB_OK | MB_ICONINFORMATION);
}

// Add/Edit Event Dialog
LRESULT CALLBACK AddEventDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            // If editing, populate fields
            if (g_edit_mode) {
                Event *e = find_event_by_id(g_edit_event_id);
                if (e) {
                    SetDlgItemText(hwnd, IDC_DESC, e->description);
                    SetDlgItemText(hwnd, IDC_LOC, e->location);
                    SetDlgItemInt(hwnd, IDC_HOUR, e->start_time.hour, FALSE);
                    SetDlgItemInt(hwnd, IDC_MIN, e->start_time.minute, FALSE);
                    SetDlgItemInt(hwnd, IDC_END_HOUR, e->end_time.hour, FALSE);
                    SetDlgItemInt(hwnd, IDC_END_MIN, e->end_time.minute, FALSE);
                    
                    CheckDlgButton(hwnd, IDC_ALL_DAY, e->is_all_day ? BST_CHECKED : BST_UNCHECKED);
                    SendDlgItemMessage(hwnd, IDC_PRIORITY, CB_SETCURSEL, e->priority, 0);
                    SendDlgItemMessage(hwnd, IDC_CATEGORY, CB_SETCURSEL, e->category, 0);
                    
                    if (e->reminder_minutes > 0) {
                        CheckDlgButton(hwnd, IDC_REMINDER, BST_CHECKED);
                        SetDlgItemInt(hwnd, IDC_REMINDER_MIN, e->reminder_minutes, FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_REMINDER_MIN), TRUE);
                    }
                    
                    if (e->is_all_day) {
                        EnableWindow(GetDlgItem(hwnd, IDC_HOUR), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_MIN), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_END_HOUR), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_END_MIN), FALSE);
                    }
                }
            }
            return TRUE;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_ALL_DAY: {
                    BOOL checked = IsDlgButtonChecked(hwnd, IDC_ALL_DAY);
                    EnableWindow(GetDlgItem(hwnd, IDC_HOUR), !checked);
                    EnableWindow(GetDlgItem(hwnd, IDC_MIN), !checked);
                    EnableWindow(GetDlgItem(hwnd, IDC_END_HOUR), !checked);
                    EnableWindow(GetDlgItem(hwnd, IDC_END_MIN), !checked);
                    return 0;
                }
                
                case IDC_REMINDER: {
                    BOOL checked = IsDlgButtonChecked(hwnd, IDC_REMINDER);
                    EnableWindow(GetDlgItem(hwnd, IDC_REMINDER_MIN), checked);
                    return 0;
                }
                
                case IDC_SAVE: {
                    char desc[MAX_DESC], loc[MAX_LOC];
                    GetDlgItemText(hwnd, IDC_DESC, desc, MAX_DESC);
                    GetDlgItemText(hwnd, IDC_LOC, loc, MAX_LOC);
                    
                    if (strlen(desc) == 0) {
                        MessageBox(hwnd, "Description cannot be empty!", "Error", MB_OK | MB_ICONERROR);
                        return 0;
                    }
                    
                    Time start = {
                        GetDlgItemInt(hwnd, IDC_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwnd, IDC_MIN, NULL, FALSE)
                    };
                    Time end = {
                        GetDlgItemInt(hwnd, IDC_END_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwnd, IDC_END_MIN, NULL, FALSE)
                    };
                    
                    // Validate times
                    if (start.hour > 23 || start.minute > 59 || end.hour > 23 || end.minute > 59) {
                        MessageBox(hwnd, "Invalid time! Hours: 0-23, Minutes: 0-59", "Error", MB_OK | MB_ICONERROR);
                        return 0;
                    }
                    
                    int all_day = IsDlgButtonChecked(hwnd, IDC_ALL_DAY);
                    if (!all_day && (start.hour * 60 + start.minute >= end.hour * 60 + end.minute)) {
                        MessageBox(hwnd, "End time must be after start time!", "Error", MB_OK | MB_ICONERROR);
                        return 0;
                    }
                    
                    Priority pri = (Priority)SendDlgItemMessage(hwnd, IDC_PRIORITY, CB_GETCURSEL, 0, 0);
                    Category cat = (Category)SendDlgItemMessage(hwnd, IDC_CATEGORY, CB_GETCURSEL, 0, 0);
                    
                    int reminder = 0;
                    if (IsDlgButtonChecked(hwnd, IDC_REMINDER)) {
                        reminder = GetDlgItemInt(hwnd, IDC_REMINDER_MIN, NULL, FALSE);
                    }
                    
                    if (g_edit_mode) {
                        // Edit existing event
                        Event *e = find_event_by_id(g_edit_event_id);
                        if (e) {
                            e->date = g_selected_date;
                            e->start_time = start;
                            e->end_time = end;
                            strncpy(e->description, desc, MAX_DESC-1);
                            e->description[MAX_DESC-1] = '\0';
                            strncpy(e->location, loc, MAX_LOC-1);
                            e->location[MAX_LOC-1] = '\0';
                            e->priority = pri;
                            e->category = cat;
                            e->is_all_day = all_day;
                            e->reminder_minutes = reminder;
                            
                            save_events();
                            update_list_view(NULL);
                            SetWindowText(hwndStatus, "Event updated successfully!");
                        }
                    } else {
                        // Create new event 
                        Event *e = create_event(g_selected_date, start, end, desc, loc, pri, cat, all_day, reminder);
                        if (e) {
                            add_event_to_list(e);
                            save_events();
                            update_list_view(NULL);
                            SetWindowText(hwndStatus, "Event added successfully!");
                        }
                    }
                    
                    DestroyWindow(hwnd);
                    hwndAddDialog = NULL;
                    g_edit_mode = 0;
                    g_edit_event_id = 0;
                    return 0;
                }
                
                case IDC_CANCEL: {
                    DestroyWindow(hwnd);
                    hwndAddDialog = NULL;
                    g_edit_mode = 0;
                    g_edit_event_id = 0;
                    return 0;
                }
            }
            break;
        }
        
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            hwndAddDialog = NULL;
            g_edit_mode = 0;
            g_edit_event_id = 0;
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void show_add_edit_event_dialog(HWND parent, int edit_mode, int event_id) {
    if (hwndAddDialog) {
        SetForegroundWindow(hwndAddDialog);
        return;
    }
    
    g_edit_mode = edit_mode;
    g_edit_event_id = event_id;
    
    static int registered = 0;
    if (!registered) {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = AddEventDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = "AddEventDialog";
        RegisterClassEx(&wc);
        registered = 1;
    }
    
    if (!edit_mode) {
        SYSTEMTIME st;
        MonthCal_GetCurSel(hwndCalendar, &st);
        g_selected_date.day = st.wDay;
        g_selected_date.month = st.wMonth;
        g_selected_date.year = st.wYear;
    } else {
        Event *e = find_event_by_id(event_id);
        if (e) {
            g_selected_date = e->date;
        }
    }
    
    hwndAddDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "AddEventDialog",
        edit_mode ? "Edit Event" : "Add New Event",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 540,
        parent, NULL, hInst, NULL
    );
    
    int y = 15;
    
    CreateWindow("STATIC", "Description:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                  125, y, 330, 25, hwndAddDialog, (HMENU)IDC_DESC, hInst, NULL);
    y += 35;
    
    CreateWindow("STATIC", "Location:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                  125, y, 330, 25, hwndAddDialog, (HMENU)IDC_LOC, hInst, NULL);
    y += 40;
    
    CreateWindow("BUTTON", "All Day Event", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                15, y, 150, 25, hwndAddDialog, (HMENU)IDC_ALL_DAY, hInst, NULL);
    y += 35;
    
    CreateWindow("STATIC", "Start Time (HH:MM):", WS_CHILD | WS_VISIBLE,
                15, y, 110, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "9", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  135, y, 50, 25, hwndAddDialog, (HMENU)IDC_HOUR, hInst, NULL);
    CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE,
                190, y+3, 10, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  205, y, 50, 25, hwndAddDialog, (HMENU)IDC_MIN, hInst, NULL);
    y += 35;
    
    CreateWindow("STATIC", "End Time (HH:MM):", WS_CHILD | WS_VISIBLE,
                15, y, 110, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "10", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  135, y, 50, 25, hwndAddDialog, (HMENU)IDC_END_HOUR, hInst, NULL);
    CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE,
                190, y+3, 10, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  205, y, 50, 25, hwndAddDialog, (HMENU)IDC_END_MIN, hInst, NULL);
    y += 40;
    
    CreateWindow("STATIC", "Priority:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    HWND hwndPriority = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                     125, y, 150, 200, hwndAddDialog, (HMENU)IDC_PRIORITY, hInst, NULL);
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Low");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Medium");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"High");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Critical");
    SendMessage(hwndPriority, CB_SETCURSEL, 1, 0);
    y += 35;
    
    CreateWindow("STATIC", "Category:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    HWND hwndCategory = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                     125, y, 150, 200, hwndAddDialog, (HMENU)IDC_CATEGORY, hInst, NULL);
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Work");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Personal");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Birthday");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Meeting");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Appointment");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Reminder");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Holiday");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Other");
    SendMessage(hwndCategory, CB_SETCURSEL, 0, 0);
    y += 35;
    
    
    CreateWindow("BUTTON", "Set Reminder", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                15, y, 120, 25, hwndAddDialog, (HMENU)IDC_REMINDER, hInst, NULL);
    HWND hwndReminderMin = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "15", 
                                         WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                                         145, y, 60, 25, hwndAddDialog, (HMENU)IDC_REMINDER_MIN, hInst, NULL);
    EnableWindow(hwndReminderMin, FALSE);
    CreateWindow("STATIC", "minutes before", WS_CHILD | WS_VISIBLE,
                210, y+3, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    y += 50;
    
    CreateWindow("BUTTON", edit_mode ? "Update Event" : "Save Event", 
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                100, y, 120, 35, hwndAddDialog, (HMENU)IDC_SAVE, hInst, NULL);
    CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                240, y, 120, 35, hwndAddDialog, (HMENU)IDC_CANCEL, hInst, NULL);
    
    SetFocus(GetDlgItem(hwndAddDialog, IDC_DESC));
    
    if (edit_mode) {
        SendMessage(hwndAddDialog, WM_INITDIALOG, 0, 0);
    }
}

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            
            // Calendar
            hwndCalendar = CreateWindowEx(0, MONTHCAL_CLASS, "",
                                         WS_CHILD | WS_VISIBLE | WS_BORDER | MCS_DAYSTATE,
                                         20, 20, 300, 300,
                                         hwnd, (HMENU)ID_CALENDAR, hInst, NULL);
            
            // Search controls
            CreateWindow("STATIC", "Search:", WS_CHILD | WS_VISIBLE,
                        340, 25, 60, 25, hwnd, NULL, hInst, NULL);
            hwndSearchBox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                          405, 22, 250, 25, hwnd, (HMENU)ID_SEARCH_BOX, hInst, NULL);
            SendMessage(hwndSearchBox, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            CreateWindow("BUTTON", "Search", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        665, 20, 80, 30, hwnd, (HMENU)ID_SEARCH, hInst, NULL);
            
            // Filter Category
            CreateWindow("STATIC", "Category:", WS_CHILD | WS_VISIBLE,
                        760, 25, 70, 25, hwnd, NULL, hInst, NULL);
            HWND hwndFilterCat = CreateWindow("COMBOBOX", "", 
                                             WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                             835, 22, 150, 200, hwnd, (HMENU)ID_FILTER_CAT, hInst, NULL);
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"All Categories");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Work");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Personal");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Birthday");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Meeting");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Appointment");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Reminder");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Holiday");
            SendMessage(hwndFilterCat, CB_ADDSTRING, 0, (LPARAM)"Other");
            SendMessage(hwndFilterCat, CB_SETCURSEL, 0, 0);
            
            // Filter Priority
            CreateWindow("STATIC", "Priority:", WS_CHILD | WS_VISIBLE,
                        1000, 25, 60, 25, hwnd, NULL, hInst, NULL);
            HWND hwndFilterPri = CreateWindow("COMBOBOX", "",
                                             WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                             1065, 22, 120, 200, hwnd, (HMENU)ID_FILTER_PRI, hInst, NULL);
            SendMessage(hwndFilterPri, CB_ADDSTRING, 0, (LPARAM)"All Priorities");
            SendMessage(hwndFilterPri, CB_ADDSTRING, 0, (LPARAM)"Low");
            SendMessage(hwndFilterPri, CB_ADDSTRING, 0, (LPARAM)"Medium");
            SendMessage(hwndFilterPri, CB_ADDSTRING, 0, (LPARAM)"High");
            SendMessage(hwndFilterPri, CB_ADDSTRING, 0, (LPARAM)"Critical");
            SendMessage(hwndFilterPri, CB_SETCURSEL, 0, 0);
            // List View
            hwndListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
                                         WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                         340, 60, 850, 480,
                                         hwnd, (HMENU)ID_LIST, hInst, NULL);
            
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            
            lvc.pszText = "ID";
            lvc.cx = 50;
            ListView_InsertColumn(hwndListView, 0, &lvc);
            
            lvc.pszText = "Date";
            lvc.cx = 100;
            ListView_InsertColumn(hwndListView, 1, &lvc);
            
            lvc.pszText = "Time";
            lvc.cx = 110;
            ListView_InsertColumn(hwndListView, 2, &lvc);
            
            lvc.pszText = "Description";
            lvc.cx = 250;
            ListView_InsertColumn(hwndListView, 3, &lvc);
            
            lvc.pszText = "Location";
            lvc.cx = 130;
            ListView_InsertColumn(hwndListView, 4, &lvc);
            
            lvc.pszText = "Priority";
            lvc.cx = 80;
            ListView_InsertColumn(hwndListView, 5, &lvc);
            
            lvc.pszText = "Category";
            lvc.cx = 100;
            ListView_InsertColumn(hwndListView, 6, &lvc);
            

            ListView_SetExtendedListViewStyle(hwndListView, 
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            
            // Buttons 
            int btn_y = 340;
            int btn_w = 140;
            int btn_h = 35;
            int btn_spacing = 10;
            
            CreateWindow("BUTTON", "Add Event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_ADD_BTN, hInst, NULL);
            
            CreateWindow("BUTTON", "Edit Event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        170, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_EDIT_BTN, hInst, NULL);
            
            btn_y += btn_h + btn_spacing;
            
            CreateWindow("BUTTON", "Delete Event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_DELETE_BTN, hInst, NULL);
            
            CreateWindow("BUTTON", "View Details", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        170, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_VIEW_DETAILS, hInst, NULL);
            
            btn_y += btn_h + btn_spacing;
            
            CreateWindow("BUTTON", "View Today", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_VIEW_TODAY, hInst, NULL);
            
            CreateWindow("BUTTON", "View All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        170, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_VIEW_ALL, hInst, NULL);
            
            btn_y += btn_h + btn_spacing;
            
            CreateWindow("BUTTON", "Export CSV", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_EXPORT, hInst, NULL);
            
            CreateWindow("BUTTON", "Backup", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        170, btn_y, btn_w, btn_h, hwnd, (HMENU)ID_BACKUP, hInst, NULL);
            
            btn_y += btn_h + btn_spacing;
            
            CreateWindow("BUTTON", "Statistics", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, btn_y, (btn_w * 2) + btn_spacing, btn_h, hwnd, (HMENU)ID_STATS, hInst, NULL);

            CreateWindow("BUTTON", "Debug", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, btn_y + 50, 290, 35, hwnd, (HMENU)9999, hInst, NULL);
            
            // Status bar
            hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, "Ready",
                                       WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                       0, 0, 0, 0, hwnd, NULL, hInst, NULL);
            

            load_events();
            
            // Debug output
            int count = 0;
            Event *e = event_list;
            while (e) {
                if (!e->deleted) count++;
                e = e->next;
            }
            
            char status[200];
            sprintf(status, "Loaded %d events. Ready.", count);
            SetWindowText(hwndStatus, status);
            
            // NOW update the list view
            update_list_view(NULL);
            
            return 0;
        }
        
        // UPDATED: Completely replaced WM_NOTIFY to fix display blocking
        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            
            // Calendar selection
            if (nmhdr->idFrom == ID_CALENDAR && nmhdr->code == MCN_SELECT) {
                LPNMSELCHANGE pSelChange = (LPNMSELCHANGE)lParam;
                Date selected = {
                    pSelChange->stSelStart.wDay,
                    pSelChange->stSelStart.wMonth,
                    pSelChange->stSelStart.wYear
                };
                update_list_view(&selected);
                
                char status[100];
                sprintf(status, "Showing events for %02d/%02d/%d", 
                       selected.day, selected.month, selected.year);
                SetWindowText(hwndStatus, status);
                return 0;
            }
            
            // Double-click calendar to add event
            if (nmhdr->idFrom == ID_CALENDAR && nmhdr->code == NM_DBLCLK) {
                show_add_edit_event_dialog(hwnd, 0, 0);
                return 0;
            }
            
            // Double-click list to view details
            if (nmhdr->idFrom == ID_LIST && nmhdr->code == NM_DBLCLK) {
                int idx = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
                if (idx != -1) {
                    LVITEM lvi = {0};
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = idx;
                    ListView_GetItem(hwndListView, &lvi);
                    show_event_details((int)lvi.lParam);
                }
                return 0;
            }
            
            // Column sorting
            if (nmhdr->idFrom == ID_LIST && nmhdr->code == LVN_COLUMNCLICK) {
                update_list_view(NULL);
                return 0;
            }
            
            // Custom draw for colors
            if (nmhdr->idFrom == ID_LIST && nmhdr->code == NM_CUSTOMDRAW) {
                LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                
                switch(lplvcd->nmcd.dwDrawStage) {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;
                        
                    case CDDS_ITEMPREPAINT: {
                        // Get the event ID from lParam
                        LVITEM lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = (int)lplvcd->nmcd.dwItemSpec;
                        ListView_GetItem(hwndListView, &lvi);
                        
                        int event_id = (int)lvi.lParam;
                        Event *e = find_event_by_id(event_id);
                        
                        if (e) {
                            // Set background color based on priority
                            lplvcd->clrTextBk = get_priority_color(e->priority);
                            lplvcd->clrText = RGB(0, 0, 0); 
                        }
                        
                        return CDRF_NEWFONT;
                    }
                    
                    default:
                        return CDRF_DODEFAULT;
                }
            }
            
            return 0;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_ADD_BTN:
                case IDM_NEW: {
                    show_add_edit_event_dialog(hwnd, 0, 0);
                    break;
                }
                
                case ID_EDIT_BTN:
                case IDM_EDIT: {
                    int idx = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
                    if (idx != -1) {
                        LVITEM lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = idx;
                        ListView_GetItem(hwndListView, &lvi);
                        show_add_edit_event_dialog(hwnd, 1, (int)lvi.lParam);
                    } else {
                        MessageBox(hwnd, "Please select an event to edit.", 
                                 "No Selection", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                
                case ID_DELETE_BTN:
                case IDM_DELETE: {
                    int idx = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
                    if (idx != -1) {
                        LVITEM lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = idx;
                        ListView_GetItem(hwndListView, &lvi);
                        
                        int id = (int)lvi.lParam;
                        Event *e = find_event_by_id(id);
                        
                        if (e) {
                            char msg[400];
                            sprintf(msg, "Delete this event?\n\n%s\n%02d/%02d/%d",
                                   e->description, e->date.day, e->date.month, e->date.year);
                            
                            if (MessageBox(hwnd, msg, "Confirm Delete",
                                          MB_YESNO | MB_ICONQUESTION) == IDYES) {
                                delete_event(id);
                                save_events();
                                update_list_view(NULL);
                                SetWindowText(hwndStatus, "Event deleted!");
                            }
                        }
                    } else {
                        MessageBox(hwnd, "Please select an event to delete.",
                                 "No Selection", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                
                case ID_VIEW_DETAILS: {
                    int idx = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
                    if (idx != -1) {
                        LVITEM lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = idx;
                        ListView_GetItem(hwndListView, &lvi);
                        show_event_details((int)lvi.lParam);
                    } else {
                        MessageBox(hwnd, "Please select an event to view details.",
                                 "No Selection", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                
                case ID_VIEW_TODAY: {
                    Date today;
                    get_today(&today);
                    
                    SYSTEMTIME st = {0};
                    st.wYear = today.year;
                    st.wMonth = today.month;
                    st.wDay = today.day;
                    MonthCal_SetCurSel(hwndCalendar, &st);
                    
                    update_list_view(&today);
                    SetWindowText(hwndStatus, "Showing today's events");
                    break;
                }
                
                case ID_VIEW_ALL:
                case IDM_REFRESH: {
                    update_list_view(NULL);
                    SetWindowText(hwndStatus, "Showing all events");
                    break;
                }
                
                case ID_SEARCH:
                case IDM_SEARCH: {
                    GetWindowText(hwndSearchBox, g_search_filter, MAX_DESC);
                    update_list_view(NULL);
                    
                    if (strlen(g_search_filter) > 0) {
                        char msg[300];
                        sprintf(msg, "Searching for: %s", g_search_filter);
                        SetWindowText(hwndStatus, msg);
                    } else {
                        SetWindowText(hwndStatus, "Search cleared");
                    }
                    break;
                }
                
                case ID_FILTER_CAT: {
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int sel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        g_category_filter = sel - 1; // -1 for "All"
                        update_list_view(NULL);
                    }
                    break;
                }
                
                case ID_FILTER_PRI: {
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int sel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        g_priority_filter = sel - 1; // -1 for "All"
                        update_list_view(NULL);
                    }
                    break;
                }
                
                case ID_EXPORT: {
                    char filename[MAX_PATH] = "events.csv";
                    OPENFILENAME ofn = {0};
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = filename;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                    ofn.lpstrDefExt = "csv";
                    
                    if (GetSaveFileName(&ofn)) {
                        export_to_csv(filename);
                        MessageBox(hwnd, "Events exported successfully!", 
                                 "Export Complete", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                
                case ID_BACKUP: {
                    backup_data();
                    break;
                }
                
                case ID_STATS: {
                    int total = 0, priorities[4] = {0}, categories[8] = {0};
                    int all_day = 0, with_reminder = 0; 
                    Event *e = event_list;
                    while (e) {
                        if (!e->deleted) {
                            total++;
                            priorities[e->priority]++;
                            categories[e->category]++;
                            if (e->is_all_day) all_day++;
                            if (e->reminder_minutes > 0) with_reminder++;
                        }
                        e = e->next;
                    }
                    
                    char stats[2000];
                    sprintf(stats, 
                           "===================================\n"
                           "        CALENDAR STATISTICS\n"
                           "===================================\n\n"
                           "OVERVIEW:\n"
                           "  Total Events: %d\n"
                           "  All-Day Events: %d\n"
                           "  Events with Reminders: %d\n\n"
                           "PRIORITY DISTRIBUTION:\n"
                           "  Critical: %d\n"
                           "  High: %d\n"
                           "  Medium: %d\n"
                           "  Low: %d\n\n"
                           "CATEGORY DISTRIBUTION:\n"
                           "  Work: %d\n"
                           "  Personal: %d\n"
                           "  Birthday: %d\n"
                           "  Meeting: %d\n"
                           "  Appointment: %d\n"
                           "  Reminder: %d\n"
                           "  Holiday: %d\n"
                           "  Other: %d\n",
                           total, all_day, with_reminder,
                           priorities[PRIORITY_CRITICAL],
                           priorities[PRIORITY_HIGH],
                           priorities[PRIORITY_MEDIUM],
                           priorities[PRIORITY_LOW],
                           categories[CAT_WORK],
                           categories[CAT_PERSONAL],
                           categories[CAT_BIRTHDAY],
                           categories[CAT_MEETING],
                           categories[CAT_APPOINTMENT],
                           categories[CAT_REMINDER],
                           categories[CAT_HOLIDAY],
                           categories[CAT_OTHER]);
                    
                    MessageBox(hwnd, stats, "Calendar Statistics", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                
                case ID_SEARCH_BOX: {
                    if (HIWORD(wParam) == EN_CHANGE) {
                        GetWindowText(hwndSearchBox, g_search_filter, MAX_DESC);
                        update_list_view(NULL);
                    }
                    break;
                }

                case 9999: {
                    debug_print_events();
                    update_list_view(NULL);
                    break;
                }
            }
            return 0;
        }
        
        case WM_SIZE: {
            SendMessage(hwndStatus, WM_SIZE, 0, 0);
            return 0;
        }
        
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        
        case WM_CLOSE: {
            int result = MessageBox(hwnd, 
                                   "Are you sure you want to exit?\n\n"
                                   "All events will be saved automatically.",
                                   "Exit Calendar Manager",
                                   MB_YESNO | MB_ICONQUESTION);
            if (result == IDYES) {
                DestroyWindow(hwnd);
            }
            return 0;
        }
        
        case WM_DESTROY: {
            save_events();
            
            // Free memory
            Event *e = event_list;
            while (e) {
                Event *next = e->next;
                free(e);
                e = next;
            }
            
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_DATE_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Register main window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "CalendarManagerEnhanced";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }
    
    // Create main window with larger size for new features
    hwndMain = CreateWindowEx(
        0,
        "CalendarManagerEnhanced",
        "📅 Calendar Manager Pro - Enhanced Edition",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1220, 680,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwndMain) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }
    
    // Create accelerators for keyboard shortcuts
    ACCEL accel[5];
    accel[0].fVirt = FCONTROL | FVIRTKEY;
    accel[0].key = 'N';
    accel[0].cmd = IDM_NEW;
    
    accel[1].fVirt = FCONTROL | FVIRTKEY;
    accel[1].key = 'E';
    accel[1].cmd = IDM_EDIT;
    
    accel[2].fVirt = FVIRTKEY;
    accel[2].key = VK_DELETE;
    accel[2].cmd = IDM_DELETE;
    
    accel[3].fVirt = FCONTROL | FVIRTKEY;
    accel[3].key = 'F';
    accel[3].cmd = IDM_SEARCH;
    
    accel[4].fVirt = FVIRTKEY;
    accel[4].key = VK_F5;
    accel[4].cmd = IDM_REFRESH;
    
    HACCEL hAccel = CreateAcceleratorTable(accel, 5);
    
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);
    
    // Show welcome message
    Date today;
    get_today(&today);
    
    int event_count = 0;
    Event *e = event_list;
    while (e) {
        if (!e->deleted) event_count++;
        e = e->next;
    }
    
    // Message loop with accelerator support
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        if (!TranslateAccelerator(hwndMain, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return (int)msg.wParam;
}

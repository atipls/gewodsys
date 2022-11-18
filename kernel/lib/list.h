
#define LIST_HEAD(type) \
    struct {            \
        type *first;    \
        type *last;     \
    }

#define LIST_ENTRY(type) \
    struct {             \
        type *next;      \
        type *prev;      \
    }

#define LIST_HEAD_INIT \
    { 0, 0 }

#define LIST_INIT(head)    \
    {                      \
        (head)->first = 0; \
        (head)->last = 0;  \
    }

#define LIST_ENTRY_INIT(entry) \
    {                          \
        (entry)->next = 0;     \
        (entry)->prev = 0;     \
    }

#define LIST_ADD(head, el, name)            \
    {                                       \
        if ((head)->first == 0) {           \
            (head)->first = (el);           \
            (head)->last = (el);            \
            (el)->name.next = 0;            \
            (el)->name.prev = 0;            \
        } else {                            \
            (head)->last->name.next = (el); \
            (el)->name.next = 0;            \
            (el)->name.prev = (head)->last; \
            (head)->last = (el);            \
        }                                   \
    }

#define LIST_ADD_FRONT(head, el, name)       \
    {                                        \
        if ((head)->first == 0) {            \
            (head)->first = (el);            \
            (head)->last = (el);             \
            (el)->name.next = 0;             \
            (el)->name.prev = 0;             \
        } else {                             \
            (head)->first->name.prev = (el); \
            (el)->name.next = (head)->first; \
            (el)->name.prev = 0;             \
            (head)->first = (el);            \
        }                                    \
    }

#define LIST_REMOVE(head, el, name)                       \
    {                                                     \
        if ((el) == (head)->first) {                      \
            if ((el) == (head)->last) {                   \
                (head)->first = 0;                        \
                (head)->last = 0;                         \
            } else {                                      \
                (el)->name.next->name.prev = 0;           \
                (head)->first = (el)->name.next;          \
                (el)->name.next = 0;                      \
            }                                             \
        } else if ((el) == (head)->last) {                \
            (el)->name.prev->name.next = 0;               \
            (head)->last = (el)->name.prev;               \
            (el)->name.prev = 0;                          \
        } else {                                          \
            (el)->name.next->name.prev = (el)->name.prev; \
            (el)->name.prev->name.next = (el)->name.next; \
            (el)->name.next = 0;                          \
            (el)->name.prev = 0;                          \
        }                                                 \
    }

#define LIST_FOREACH(var, head, name) \
    for ((var) = ((head)->first); (var); (var) = ((var)->name.next))

#define RLIST_FOREACH(var, el, name) \
    for ((var) = (el); (var); (var) = ((var)->name.next))
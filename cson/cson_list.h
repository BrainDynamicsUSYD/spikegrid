/*
  This file is a "supermacro", intended to be included multiple times
  by related client code.

  Inputs:

  #define LIST_T list_type
  #define VALUE_T value_type

  Additionally, define VALUE_T_IS_PTR to 1 if VALUE_T is
  pointer-qualified. This removes some routines, but is needed
  for handling (T**) lists propertly.

  list_type must be a struct with at least the following members:

  struct list_type {
  VALUE_T list; // or (VALUE_T*) if VALUE_T_IS_PTR
  unsigned int count; // number of used entries in the list
  unsigned int alloced; // number of items allocated in the list
  };

  Additional members are optional, and unused by this code.
  
  This file provides routines for (de)allocating, growing,
  visiting, and cleaning up list contents.

  The generated functions are named LIST_T_funcname(),
  e.g. list_type_reserve().
*/

#if !defined(VALUE_T_IS_PTR)
#  define VALUE_T_IS_PTR 0
#endif
#define SYM2(X,Y) X ## Y
#define SYM(X,Y) SYM2(X,Y)
#define STR2(T) # T
#define STR(T) STR2(T)

/**
   Possibly reallocates self->list, changing its size. This function
   ensures that self->list has at least n entries. If n is 0 then
   the list is deallocated (but the self object is not), BUT THIS DOES NOT
   DO ANY TYPE-SPECIFIC CLEANUP of the items. If n is less than or equal
   to self->alloced then there are no side effects. If n is greater
   than self->alloced, self->list is reallocated and self->alloced
   is adjusted to be at least n (it might be bigger - this function may
   pre-allocate a larger value).

   Passing an n of 0 when self->alloced is 0 is a no-op.

   Newly-allocated items will be initialized with NULL bytes.
   
   Returns the total number of items allocated for self->list.  On
   success, the value will be equal to or greater than n (in the
   special case of n==0, 0 is returned). Thus a return value smaller
   than n is an error. Note that if n is 0 or self is NULL then 0 is
   returned.

   The return value should be used like this:

   @code
   unsigned int const n = number of bytes to allocate;
   if( n > my_list_t_reserve( myList, n ) ) { ... error ... }
   // Or the other way around:
   if( my_list_t_reserve( myList, n ) < n ) { ... error ... }
   @endcode
*/
unsigned int SYM(LIST_T,_reserve)( LIST_T * self, unsigned int n )
{
    if( !self ) return 0;
    else if(0 == n)
    {
        if(0 == self->alloced) return 0;
        cson_free(self->list, STR(SYM(LIST_T,_reserve)));
        self->list = NULL;
        self->alloced = self->count = 0;
        return 0;
    }
    else if( self->alloced >= n )
    {
        return self->alloced;
    }
    else
    {
        size_t const sz = sizeof(VALUE_T) * n;
        VALUE_T * m = (VALUE_T*)cson_realloc( self->list, sz, STR(SYM(LIST_T,_reserve)) );
        if( ! m ) return self->alloced;
        /* Zero out the new elements... */
        memset( m + self->alloced, 0, (sizeof(VALUE_T)*(n-self->alloced)));
        self->alloced = n;
        self->list = m;
        return n;
    }
}

/**
   Appends a bitwise copy of cp to self->list, expanding the list as
   necessary and adjusting self->count.

   On success, ownership of cp

   Returns 0 on success.
*/
int SYM(LIST_T,_append)( LIST_T * self, VALUE_T cp )
{
    if( !self || !cp ) return cson_rc.ArgError;
    else if( self->alloced > SYM(LIST_T,_reserve)(self, self->count+1) )
    {
        return cson_rc.AllocError;
    }
    else
    {
        self->list[self->count++] = cp;
        return 0;
    }
}

/**
   For each item in self->list, visitor(item,visitorState) is called.
   The item is owned by self. The visitor function MUST NOT free the
   item, but may manipulate its contents if application rules do not
   specify otherwise.

   Returns 0 on success, non-0 on error.

   If visitor() returns non-0 then looping stops and that code is
   returned.
*/
int SYM(LIST_T,_visit)( LIST_T * self,
#if VALUE_T_IS_PTR
                        int (*visitor)(VALUE_T obj, void * visitorState ),
#else
                        int (*visitor)(VALUE_T * obj, void * visitorState ),
#endif
                        void * visitorState )
{
    int rc = cson_rc.ArgError;
    if( self && visitor )
    {
        unsigned int i = 0;
        for( rc = 0; (i < self->count) && (0 == rc); ++i )
        {
#if VALUE_T_IS_PTR
            VALUE_T obj = self->list[i];
#else
            VALUE_T * obj = &self->list[i];
#endif
            if(obj) rc = visitor( obj, visitorState );
        }
    }
    return rc;
}

/**
   For each item in self->list, cleaner(item) is called to free any
   resources owned by that item. The item itself MUST NOT be deleted
   by that function, only memory the item owns. After cleaning used
   items, self is emptied and must be freed (if necessary) by the
   caller.

   Possible TODO: reimplement this in terms of the visit routine. That
   requires adding an extraneous callback for the visitor function,
   though.
*/
void SYM(LIST_T,_clean)( LIST_T * self,
#if VALUE_T_IS_PTR
                         void (*cleaner)(VALUE_T obj)
#else
                         void (*cleaner)(VALUE_T * obj)
#endif
                         )
{
    if( self && cleaner && self->count )
    {
        unsigned int i = 0;
        for( ; i < self->count; ++i )
        {
#if VALUE_T_IS_PTR
            VALUE_T obj = self->list[i];
#else
            VALUE_T * obj = &self->list[i];
#endif
            if(obj) cleaner(obj);
        }
    }
    SYM(LIST_T,_reserve)(self,0);
}

#if !VALUE_T_IS_PTR
/**
   Reduces self->count by 1. Returns 0 on success.
   Errors include:

   (!self) or (0 == self->count)
*/
int SYM(LIST_T,_pop_back)( LIST_T * self, void (*cleaner)(VALUE_T * obj) )
{
    if( !self ) return cson_rc.ArgError;
    else if( ! self->count ) return cson_rc.RangeError;
    else
    {
        unsigned int const ndx = --self->count;
        VALUE_T * val = &self->list[ndx];
        if(val && cleaner) cleaner(val);
        return 0;
    }
}
#endif
#undef SYM
#undef SYM2
#undef LIST_T
#undef VALUE_T
#undef VALUE_T_IS_PTR
#undef STR
#undef STR2

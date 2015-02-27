
/**
    FormToJSON is a utility to serialize HTML forms to JSON objects.

    It transforms HTML forms to a higher-level structure which looks
    like this:

    { // form field data.
        "field1":"value 1",
        "field2":14,
        "multiCheckboxOrMultiselect": ["list","of","values"]
    }

    Each field has the same name as the HTML form element. Values 
    which "look numeric" will be converted from strings to their 
    numeric values. Multiple checkboxes with the same name, or 
    SELECT element with the MULTIPLE attribute, will cause an array 
    of values to be generated, one value per selected entry (even if 
    only one item is selected).

    Each serializable form element in the HTML form must have the CSS
    class FormToJSON.elementCssClass. Elements without that class
    are skipped (under the assumption that we don't want to, or cannot,
    serialize them).

    The default implementation requires jQuery but it can easily
    be modified to work with other DOM element selector APIs.

    It is used like this:
    
    var obj = FormToJSON.formToJSON('#myForm');
    
    The returned object can then be passed to JSON.stringify().

    Author: Stephan Beal (http://wanderinghorse.net)
    License: Public Domain

*/
var FormToJSON = {
    /**
        CSS class which to-be-serialized form fields should have.
    */
    elementCssClass:'FormToJSON',
    /**
        This function "selects" DOM elements based on the "selector" X
        and the context object CX. The default impl is simply a proxy around
        jQuery(X) or jQuery(X,CX), but custom impls can use whatever selector
        back-end they like, as long as they are "compatible enough" with jQuery.
    */
    select:function(X,CX)
    {
        return (arguments.length==1) ? jQuery(X) : jQuery(X,CX);
    },
    /** @internal

    */
    internals:{
        /** Regex matching whole-string integers. */
        rxInt: /^[-+]?\d+$/,
        /** Regex matching whole-string doubles. */
        rxDbl: /^[-+]?\d+\.\d+$/
    },
    /**
        If val is a string which looks like an integer or float then returns the
        intified/doublified version of val, else returns val as-is. This is used
        for converting number-holding form field values to numeric values.
    */
    valueOf:function(val)
    {
        if( !val ) return val;
        if( ('string' === typeof val) || (val instanceof String) ) {
            var m = this.internals.rxInt.exec(val);
            if( m ) return parseInt(val);
            m = this.internals.rxDbl.exec(val);
            if( m ) return parseFloat(val);
        }
        return val;
    },
    /**
        Creates a JSON-compatible object from the given
        form element, which may be a DOM element or selector string
        for a single form (e.g. '#myForm'). The form's 'name' attribute
        must be the name used by the server-side JsonForm dispatcher to
        dispatch the inbound data to some back-end handler which implements
        the form's logic.

        Only form elements with the class JsonForm.elementCssClass
        are processed here.

        The resulting object follows the JsonForm-conventional structure
        and its 'fields' member mirrors the forms' elements. e.g. a form with
        fields a, b, and c might product:

        "fields":{"a":12, "b":"hi, world", "c":42.24}

        Multiple checked checkboxes with same name, or a SELECT list with the MULTIPLE
        attribute, causes the key/value pair to be a key/array-of-values pair. This is
        true even if only one of those items is checked (for consistency on the server side).
        e.g. if a form has multiple checkboxes named "check" and 2 of them are set, that
        might generate:

        "fields":{... "check":[ "hi", "world" ] ... }

        Likewise, for consistency, if only one of them is checked we still generate
        and array instead of a simple key/value pair:

        "fields":{... "check":[ "hi" ] ... }    

        Unchecked checkboxes and radio buttons do NOT produce any entries in the output
        object.

        Returns a JSON-able object.

        Maintenance reminders:

        - This implementation requires that this.select(X,Y) be functionally equivalent
        to jQuery(X,Y).

    */
    formToJSON:function(formElem)
    {

        var form = this.select(formElem);
        if( 1 != form.length )
        {
            alert("Expected exactly 1 form to match selector "+formElem+", but got "+form.length+".");
            return null;
        }
        var formName = form.attr('name');
        if( ! formName ) {
            throw new Exception("Form requires a name attribute which corresponds to a server-side form handler.");
        }
        var obj = {};
        var sel;
        var k, i, elems, e, v;
        var ename;
        var combovals = {};
        /* We can handle "simple" field element types uniformly... */
        var typelist = ['[type=text]','textarea',':radio:checked'];
        for( k in typelist )
        {
            k = typelist[k];
            sel = k+'.'+JsonForm.elementCssClass
                ;
            elems = this.select(sel,form);
            for( i = 0; i < elems.length; ++i ) {
                e = this.select(elems.get(i));
                ename = e.attr('name');
                obj[ename] = this.valueOf( e.val() );
            }
        }
        typelist = ['select'];
        for( k in typelist )
        {
            k = typelist[k];
            sel = k+'.'+JsonForm.elementCssClass;
            elems = this.select(sel,form);
            for( i = 0; i < elems.length; ++i ) {
                e = this.select(elems.get(i));
                ename = e.attr('name');
                v = this.valueOf(e.val());
                if( v instanceof Array )
                { /* Multi-select option. Remember this and defer processing until later. */
                    v = combovals[ename];
                    if( undefined === v )
                    {
                        v = combovals[ename] = [];
                    }
                    v.push(e);
                    continue;
                }
                else
                {
                    obj[ename] = e.val();
                }
            }
        }
        if(1)
        { /* Radiobuttons and checkboxes need special handling: only CHECKED
            objects produce a value in the output object. This is consistent with
            normal form posting semantics. Multiple checkboxes with the same names
            need to generate an array of values (one per checked copy). For consistency's
            sake, if multiple checkboxes have the same name, we always produce an array
            as output even if only one of those checkboxes is checked. This avoids
            special-case code for the single-value case in their handling.
          */
            sel = ':checkbox.'+JsonForm.elementCssClass;
            elems = this.select(sel+':checked',form);
            for( i = 0; i < elems.length; ++i ) {
                e = this.select(elems.get(i));
                ename = e.attr('name');
                if( this.select(sel+'[name='+ename+']', form).length > 1 )
                { /* Multiple checkboxes with the same name. Remember this and
                     defer processing until later.
                    */
                    v = combovals[ename];
                    if( undefined === v )
                    {
                        v = combovals[ename] = [];
                    }
                    v.push(e);
                    continue;
                }
                obj[ename] = this.valueOf( e.val() );
            }
        }
        /* Now process any like-named checkboxes and multi-select lists... */
        var comboout;
        for( ename in combovals ) {
            if( ! combovals.hasOwnProperty(ename) ) {
                    // workaround for broken/buggy libs which extend core JS classes.
                continue;
            }
            elems = combovals[ename];
            comboout = [];
            for( i = 0; i < elems.length; ++i )
            {
                e = elems[i];
                v = e.val();
                if( v instanceof Array )
                {
                    /*
                    Reminder to self: we might want to do:

                    comboout = v;
                    break;

                    instead.
                    */
                    for( var x = 0; x < v.length; ++x )
                    {
                        comboout.push( this.valueOf( v[x] ) );
                    }
                }
                else
                {
                    comboout.push( this.valueOf( v ) );
                }
            }
            obj[ename] = comboout;
        }
        return obj;
    }
};

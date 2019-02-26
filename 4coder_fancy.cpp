/*
* Fancy string - immediate mode renderer for colored strings
*/

// TOP

static Fancy_Color
fancy_blend(id_color a, f32 t, id_color b){
    Fancy_Color result = {};
    result.index_a = (u16)a;
    result.index_b = (u16)b;
    result.table_a = 1;
    result.table_b = 1;
    result.c_b = (u8)(clamp(0, 255.0f*t, 255.0f));
    result.c_a = 255 - result.c_b;
    return(result);
}

static Fancy_Color
fancy_id(id_color a){
    Fancy_Color result = {};
    result.index_a = (u16)a;
    result.index_b = 0;
    result.table_a = 1;
    result.table_b = 0;
    result.c_a = 255;
    result.c_b = 0;
    return(result);
}

static Fancy_Color
fancy_rgba(argb_color color){
    Fancy_Color result = {};
    result.rgba = color;
    result.code = 0;
    return(result);
}

static Fancy_Color
fancy_rgba(f32 r, f32 g, f32 b, f32 a){
    Fancy_Color result = fancy_rgba(pack_color4(V4(r, g, b, a)));
    return(result);
}

static Fancy_Color
fancy_resolve_to_rgba(Application_Links *app, Fancy_Color source){
    if (source.code != 0){
        Vec4 a = unpack_color4(finalize_color(app, source.index_a));
        Vec4 b = unpack_color4(finalize_color(app, source.index_b));
        
        f32 ca = (f32)source.c_a/255.0f;
        f32 cb = (f32)source.c_b/255.0f;
        
        Vec4 value = ca*a + cb*b;
        
        source.rgba = pack_color4(value);
        source.code = 0;
    }
    return(source);
}

static Fancy_Color
pass_through_fancy_color(void){
    Fancy_Color result = {};
    return(result);
}

static int_color
int_color_from(Application_Links *app, Fancy_Color source){
    int_color result = {};
    if ((source.c_a == 255) && (source.c_b == 0)){
        result = source.index_a;
    }
    else{
        source = fancy_resolve_to_rgba(app, source);
        result = source.rgba;
    }
    return(result);
}

static bool32
is_valid(Fancy_Color source){
    bool32 result = !((source.code == 0) && (source.rgba == 0));
    return(result);
}

static void
fancy_string_list_push(Fancy_String_List *list, Fancy_String *string){
    list->last = (list->last ? list->last->next : list->first) = string;
}

static Fancy_String *
push_fancy_string(Arena *arena, Fancy_String_List *list, Fancy_Color fore, Fancy_Color back, String value){
    Fancy_String *result = push_array(arena, Fancy_String, 1);
    memset(result, 0, sizeof(*result));
    
    result->value = string_push_copy(arena, value);
    result->fore = fore;
    result->back = back;
    
    if (list != 0){
        fancy_string_list_push(list, result);
    }
    
    return(result);
}

static Fancy_String *
push_fancy_string(Arena *arena, Fancy_Color fore, Fancy_Color back, String value){
    return(push_fancy_string(arena, 0, fore, back, value));
}

static Fancy_String *
push_fancy_string(Arena *arena, Fancy_String_List *list, Fancy_Color fore, String value){
    return(push_fancy_string(arena, list, fore, pass_through_fancy_color(), value));
}

static Fancy_String *
push_fancy_string(Arena *arena, Fancy_Color fore, String value){
    return(push_fancy_string(arena, 0, fore, pass_through_fancy_color(), value));
}

static Fancy_String *
push_fancy_string(Arena *arena, Fancy_String_List *list, String value){
    return(push_fancy_string(arena, list, pass_through_fancy_color(), pass_through_fancy_color(), value));
}

static Fancy_String *
push_fancy_string(Arena *arena, String value){
    return(push_fancy_string(arena, 0, pass_through_fancy_color(), pass_through_fancy_color(), value));
}

static Fancy_String*
push_fancy_vstringf(Arena *arena, Fancy_String_List *list, Fancy_Color fore, Fancy_Color back, char *format, va_list args){
    // TODO(casey): Allen, ideally we would have our own formatter here that just outputs into a buffer and can't ever "run out of space".
    char temp[1024];
    int32_t length = vsprintf(temp, format, args);
    Fancy_String *result = 0;
    if (length > 0){
        result = push_fancy_string(arena, list, fore, back, make_string(temp, length));
    }
    return(result);
}

static Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_String_List *list, Fancy_Color fore, Fancy_Color back, char *format, ...){
    va_list args;
    va_start(args, format);
    Fancy_String *result = push_fancy_vstringf(arena, list, fore, back, format, args);
    va_end(args);
    return(result);
}

static Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_String_List *list, Fancy_Color fore, char *format, ...){
    va_list args;
    va_start(args, format);
    Fancy_String *result = push_fancy_vstringf(arena, list, fore, pass_through_fancy_color(), format, args);
    va_end(args);
    return(result);
}

static Fancy_String*
push_fancy_stringf(Arena *arena, Fancy_String_List *list, char *format, ...){
    va_list args;
    va_start(args, format);
    Fancy_String *result = push_fancy_vstringf(arena, list, pass_through_fancy_color(), pass_through_fancy_color(), format, args);
    va_end(args);
    return(result);
}

static Fancy_String_List
fancy_string_list_single(Fancy_String *fancy_string){
    Fancy_String_List list = {};
    list.first = fancy_string;
    list.last = fancy_string;
    return(list);
}

static Vec2
draw_fancy_string(Application_Links *app, Face_ID font_id, Fancy_String *string, Vec2 P,
                  int_color fore, int_color back, u32 flags, Vec2 dP){
    for (;string != 0;
         string = string->next){
        Face_ID use_font_id = (string->font_id) ? string->font_id : font_id;
        int_color use_fore = is_valid(string->fore) ? int_color_from(app, string->fore) : fore;
        
        f32 adv = get_string_advance(app, use_font_id, string->value);
        
        // TODO(casey): need to fill the background here, but I don't know the line height,
        // and I can't actually render filled shapes, so, like, I can't properly do dP :(
        
        P += (string->pre_margin)*dP;
        draw_string(app, use_font_id, string->value, P, use_fore, flags, dP);
        P += (adv + string->post_margin)*dP;
    }
    return(P);
}

// BOTTOM

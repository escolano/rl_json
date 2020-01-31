#include "rl_jsonInt.h"

Tcl_Obj* JSON_NewJSONObj(Tcl_Interp* interp, Tcl_Obj* from) //{{{
{
	return as_json(interp, from);
}

//}}}
int JSON_NewJStringObj(Tcl_Interp* interp, Tcl_Obj* string, Tcl_Obj** new) //{{{
{
	replace_tclobj(new, JSON_NewJvalObj(JSON_STRING, string));

	return TCL_OK;
}

//}}}
int JSON_NewJNumberObj(Tcl_Interp* interp, Tcl_Obj* number, Tcl_Obj** new) //{{{
{
	Tcl_Obj*			forced = NULL;
	struct interp_cx*	l = Tcl_GetAssocData(interp, "rl_json", NULL);

	TEST_OK(force_json_number(interp, l, number, &forced));
	replace_tclobj(new, JSON_NewJvalObj(JSON_NUMBER, forced));
	release_tclobj(&forced);

	return TCL_OK;
}

//}}}
int JSON_NewJBooleanObj(Tcl_Interp* interp, Tcl_Obj* boolean, Tcl_Obj** new) //{{{
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "rl_json", NULL);
	int					bool;

	TEST_OK(Tcl_GetBooleanFromObj(interp, boolean, &bool));
	replace_tclobj(new, bool ? l->json_true : l->json_false);

	return TCL_OK;
}

//}}}
int JSON_NewJNullObj(Tcl_Interp* interp, Tcl_Obj* *new) //{{{
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "rl_json", NULL);

	replace_tclobj(new, l->json_null);

    return TCL_OK;
}

//}}}
int JSON_NewJObjectObj(Tcl_Interp* interp, Tcl_Obj** new) //{{{
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "rl_json", NULL);

	replace_tclobj(new, JSON_NewJvalObj(JSON_OBJECT, l->tcl_empty_dict));

	return TCL_OK;
}

//}}}
int JSON_NewJArrayObj(Tcl_Interp* interp, int objc, Tcl_Obj* objv[], Tcl_Obj** new) //{{{
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "rl_json", NULL);

	if (objc == 0) {
		replace_tclobj(new, JSON_NewJvalObj(JSON_OBJECT, l->tcl_empty_list));
	} else {
		int		i;

		for (i=0; i<objc; i++) TEST_OK(JSON_ForceJSON(interp, objv[i]));

		replace_tclobj(new, JSON_NewJvalObj(JSON_OBJECT, Tcl_NewListObj(objc, objv)));
	}

	return TCL_OK;
}

//}}}
int JSON_NewTemplateObj(Tcl_Interp* interp, enum json_types type, Tcl_Obj* key, Tcl_Obj** new) //{{{
{
	if (!type_is_dynamic(type)) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Requested type is not a valid template type: %s", type_names_int[type]));
		return TCL_ERROR;
	}

	replace_tclobj(new, JSON_NewJvalObj(type, key));

	return TCL_OK;	
}

//}}}
int JSON_ForceJSON(Tcl_Interp* interp, Tcl_Obj* obj) // Force a conversion to a JSON objtype, or throw an exception {{{
{
	Tcl_ObjIntRep*	ir;
	enum json_types	type;

	TEST_OK(JSON_GetIntrepFromObj(interp, obj, &type, &ir));

	return TCL_OK;
}

//}}}

enum json_types JSON_GetJSONType(Tcl_Obj* obj) //{{{
{
	Tcl_ObjIntRep*	ir = NULL;
	enum json_types	t;

	for (t=JSON_OBJECT; t<JSON_TYPE_MAX && ir==NULL; t++)
		ir = Tcl_FetchIntRep(obj, g_objtype_for_type[t]);

	return (ir == NULL) ? JSON_UNDEF : t-1;
}

//}}}
int JSON_GetObjFromJStringObj(Tcl_Interp* interp, Tcl_Obj* jstringObj, Tcl_Obj** stringObj) //{{{
{
	enum json_types	type;
	Tcl_Obj*		val = NULL;

	TEST_OK(JSON_GetJvalFromObj(interp, jstringObj, &type, &val));

	if (type_is_dynamic(type)) {
		replace_tclobj(stringObj, Tcl_ObjPrintf("%s%s", get_dyn_prefix(type), Tcl_GetString(val)));
		return TCL_OK;
	} else if (type == JSON_STRING) {
		replace_tclobj(stringObj, val);
		return TCL_OK;
	} else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Asked for string from json string but supplied json %s", get_type_name(type)));
		return TCL_ERROR;
	}
}

//}}}
int JSON_GetObjFromJNumberObj(Tcl_Interp* interp, Tcl_Obj* jnumberObj, Tcl_Obj** numberObj) //{{{
{
	enum json_types	type;
	Tcl_Obj*		val = NULL;

	TEST_OK(JSON_GetJvalFromObj(interp, jnumberObj, &type, &val));

	if (type != JSON_NUMBER) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Asked for number from json number but supplied json %s", get_type_name(type)));
		return TCL_ERROR;
	}

	replace_tclobj(numberObj, val);

	return TCL_OK;
}

//}}}
int JSON_GetObjFromJBooleanObj(Tcl_Interp* interp, Tcl_Obj* jbooleanObj, Tcl_Obj** booleanObj) //{{{
{
	enum json_types	type;
	Tcl_Obj*		val = NULL;

	TEST_OK(JSON_GetJvalFromObj(interp, jbooleanObj, &type, &val));

	if (type != JSON_BOOL) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Asked for boolean from json boolean but supplied json %s", get_type_name(type)));
		return TCL_ERROR;
	}

	replace_tclobj(booleanObj, val);

	return TCL_OK;
}

//}}}
int JSON_JArrayObjAppendElement(Tcl_Interp* interp, Tcl_Obj* arrayObj, Tcl_Obj* elem) //{{{
{
	enum json_types	type;
	Tcl_ObjIntRep*	ir = NULL;
	Tcl_Obj*		val = NULL;

	if (Tcl_IsShared(arrayObj)) {
		// Tcl_Panic?
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("JSON_JArrayObjAppendElement called with shared object"));
		return TCL_ERROR;
	}

	TEST_OK(JSON_GetIntrepFromObj(interp, arrayObj, &type, &ir));
	val = get_unshared_val(ir);

	if (type != JSON_ARRAY) // Turn it into one by creating a new array with a single element containing the old value
		TEST_OK(JSON_SetIntRep(arrayObj, JSON_ARRAY, Tcl_NewListObj(1, &val)));

	TEST_OK(Tcl_ListObjAppendElement(interp, val, as_json(interp, elem)));

	release_tclobj((Tcl_Obj**)&ir->twoPtrValue.ptr2);
	Tcl_InvalidateStringRep(arrayObj);

	return TCL_OK;
}

//}}}
int JSON_JArrayObjAppendList(Tcl_Interp* interp, Tcl_Obj* arrayObj, Tcl_Obj* elems /* a JArrayObj or ListObj */ )
{
	THROW_ERROR("Not implemented yet");
}

int JSON_SetJArrayObj(Tcl_Interp* interp, Tcl_Obj* obj, int objc, Tcl_Obj* objv[])
{
	THROW_ERROR("Not implemented yet");
}

int JSON_JArrayObjGetElements(Tcl_Interp* interp, Tcl_Obj* arrayObj, int* objc, Tcl_Obj** objv)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_JArrayObjIndex(Tcl_Interp* interp, Tcl_Obj* arrayObj, int index, Tcl_Obj** elem)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_JArrayObjReplace(Tcl_Interp* interp, Tcl_Obj* arrayObj, int first, int count, int objc, Tcl_Obj* objv[])
{
	THROW_ERROR("Not implemented yet");
}

// TODO: JObject interface, similar to DictObj

int JSON_Get(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, Tcl_Obj** res)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Extract(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, Tcl_Obj** res)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Exists(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, int* exists)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Set(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj *path, Tcl_Obj* replacement) //{{{
{
	int				i, pathc;
	enum json_types	type, newtype;
	Tcl_ObjIntRep*	ir = NULL;
	Tcl_Obj*		val = NULL;
	Tcl_Obj*		step;
	Tcl_Obj*		src;
	Tcl_Obj*		target;
	Tcl_Obj*		newval;
	Tcl_Obj*		rep = NULL;
	Tcl_Obj**		pathv = NULL;

	src = Tcl_ObjGetVar2(interp, obj, NULL, 0);
	if (src == NULL) {
		src = Tcl_ObjSetVar2(interp, obj, NULL, JSON_NewJvalObj(JSON_OBJECT, Tcl_NewDictObj()), TCL_LEAVE_ERR_MSG);
	}

	if (Tcl_IsShared(src)) {
		src = Tcl_ObjSetVar2(interp, obj, NULL, Tcl_DuplicateObj(src), TCL_LEAVE_ERR_MSG);
		if (src == NULL)
			return TCL_ERROR;
	}

	/*
	fprintf(stderr, "JSON_Set, obj: \"%s\", src: \"%s\"\n",
			Tcl_GetString(obj), Tcl_GetString(src));
			*/
	target = src;

	TEST_OK(JSON_GetIntrepFromObj(interp, target, &type, &ir));
	val = get_unshared_val(ir);

	TEST_OK(Tcl_ListObjGetElements(interp, path, &pathc, &pathv));

	// Walk the path as far as it exists in src
	//fprintf(stderr, "set, initial type %s\n", type_names[type]);
	for (i=0; i<pathc; i++) {
		step = pathv[i];
		//fprintf(stderr, "looking at step %s, cx type: %s\n", Tcl_GetString(step), type_names_int[type]);

		switch (type) {
			case JSON_UNDEF: //{{{
				THROW_ERROR("Found JSON_UNDEF type jval following path");
				//}}}
			case JSON_OBJECT: //{{{
				TEST_OK(Tcl_DictObjGet(interp, val, step, &target));
				if (target == NULL) {
					//fprintf(stderr, "Path element %d: \"%s\" doesn't exist creating a new key for it and storing a null\n",
					//		i, Tcl_GetString(step));
					target = JSON_NewJvalObj(JSON_NULL, NULL);
					TEST_OK(Tcl_DictObjPut(interp, val, step, target));
					i++;
					goto followed_path;
				}
				if (Tcl_IsShared(target)) {
					//fprintf(stderr, "Path element %d: \"%s\" exists but the TclObj is shared (%d), replacing it with an unshared duplicate\n",
					//		i, Tcl_GetString(step), target->refCount);
					target = Tcl_DuplicateObj(target);
					TEST_OK(Tcl_DictObjPut(interp, val, step, target));
				}
				break;
				//}}}
			case JSON_ARRAY: //{{{
				{
					int			ac, index_str_len, ok=1;
					long		index;
					const char*	index_str;
					char*		end;
					Tcl_Obj**	av;

					TEST_OK(Tcl_ListObjGetElements(interp, val, &ac, &av));
					//fprintf(stderr, "descending into array of length %d\n", ac);

					if (Tcl_GetLongFromObj(NULL, step, &index) != TCL_OK) {
						// Index isn't an integer, check for end(+/-int)?
						index_str = Tcl_GetStringFromObj(step, &index_str_len);
						if (index_str_len < 3 || strncmp("end", index_str, 3) != 0)
							ok = 0;

						if (ok) {
							index = ac-1;
							if (index_str_len >= 4) {
								if (index_str[3] != '-' && index_str[3] != '+') {
									ok = 0;
								} else {
									// errno is magically thread-safe on POSIX
									// systems (it's thread-local)
									errno = 0;
									index += strtol(index_str+3, &end, 10);
									if (errno != 0 || *end != 0)
										ok = 0;
								}
							}
						}

						if (!ok)
							THROW_ERROR("Expected an integer index or end(+/-integer)?, got ", Tcl_GetString(step));

						//fprintf(stderr, "Resolved index of %ld from \"%s\"\n", index, index_str);
					} else {
						//fprintf(stderr, "Explicit index: %ld\n", index);
					}

					if (index < 0) {
						// Prepend element to the array
						target = JSON_NewJvalObj(JSON_NULL, NULL);
						TEST_OK(Tcl_ListObjReplace(interp, val, -1, 0, 1, &target));

						i++;
						goto followed_path;
					} else if (index >= ac) {
						int			new_i;
						for (new_i=ac; new_i<index; new_i++) {
							TEST_OK(Tcl_ListObjAppendElement(interp, val,
										JSON_NewJvalObj(JSON_NULL, NULL)));
						}
						target = JSON_NewJvalObj(JSON_NULL, NULL);
						TEST_OK(Tcl_ListObjAppendElement(interp, val, target));

						i++;
						goto followed_path;
					} else {
						target = av[index];
						if (Tcl_IsShared(target)) {
							target = Tcl_DuplicateObj(target);
							TEST_OK(Tcl_ListObjReplace(interp, val, index, 1, 1, &target));
						}
						//fprintf(stderr, "extracted index %ld: (%s)\n", index, Tcl_GetString(target));
					}
				}
				break;
				//}}}
			case JSON_STRING:
			case JSON_NUMBER:
			case JSON_BOOL:
			case JSON_NULL:
			case JSON_DYN_STRING:
			case JSON_DYN_NUMBER:
			case JSON_DYN_BOOL:
			case JSON_DYN_JSON:
			case JSON_DYN_TEMPLATE:
			case JSON_DYN_LITERAL:
				THROW_ERROR("Attempt to index into atomic type ", get_type_name(type), " at path key \"", Tcl_GetString(step), "\"");
				/*
				i++;
				goto followed_path;
				*/
			default:
				THROW_ERROR("Unhandled type: ", Tcl_GetString(Tcl_NewIntObj(type)));
		}

		TEST_OK(JSON_GetIntrepFromObj(interp, target, &type, &ir));
		val = get_unshared_val(ir);
	}

	goto set_val;

followed_path:
	TEST_OK(JSON_GetIntrepFromObj(interp, target, &type, &ir));
	val = get_unshared_val(ir);

	// target points at the (first) object to replace.  It and its internalRep
	// are unshared

	// If any path elements remain then they need to be created as object
	// keys
	//fprintf(stderr, "After walking path, %d elements remain to be created\n", pathc-i);
	for (; i<pathc; i++) {
		//fprintf(stderr, "create walk %d: %s, cx type: %s\n", i, Tcl_GetString(pathv[i]), type_names_int[type]);
		if (type != JSON_OBJECT) {
			//fprintf(stderr, "Type isn't JSON_OBJECT: %s, replacing with a JSON_OBJECT\n", type_names_int[type]);
			if (val != NULL)
				Tcl_DecrRefCount(val);
			val = Tcl_NewDictObj();
			TEST_OK(JSON_SetIntRep(target, JSON_OBJECT, val));
		}

		target = JSON_NewJvalObj(JSON_OBJECT, Tcl_NewDictObj());
		//fprintf(stderr, "Adding key \"%s\"\n", Tcl_GetString(pathv[i]));
		TEST_OK(Tcl_DictObjPut(interp, val, pathv[i], target));
		TEST_OK(JSON_GetJvalFromObj(interp, target, &type, &val));
		//fprintf(stderr, "Newly added key \"%s\" is of type %s\n", Tcl_GetString(pathv[i]), type_names_int[type]);
		// This was just created - it can't be shared
	}

set_val:
	//fprintf(stderr, "Reached end of path, calling JSON_SetIntRep for replacement value %s (%s), target is %s\n",
	//		type_names_int[newtype], Tcl_GetString(replacement), type_names_int[type]);
	rep = as_json(interp, replacement);

	TEST_OK(JSON_GetJvalFromObj(interp, rep, &newtype, &newval));
	TEST_OK(JSON_SetIntRep(target, newtype, newval));

	Tcl_InvalidateStringRep(src);

	if (interp)
		Tcl_SetObjResult(interp, src);

	return TCL_OK;
}

//}}}
int JSON_Unset(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj *path) //{{{
{
	enum json_types	type;
	int				i;
	Tcl_Obj*		val;
	Tcl_Obj*		step;
	Tcl_Obj*		src;
	Tcl_Obj*		target;
	int				pathc;
	Tcl_Obj**		pathv = NULL;

	src = Tcl_ObjGetVar2(interp, obj, NULL, TCL_LEAVE_ERR_MSG);
	if (src == NULL)
		return TCL_ERROR;

	TEST_OK(Tcl_ListObjGetElements(interp, path, &pathc, &pathv));

	if (pathc == 0) {
		Tcl_SetObjResult(interp, src);
		return TCL_OK;	// Do Nothing Gracefully
	}

	if (Tcl_IsShared(src)) {
		src = Tcl_ObjSetVar2(interp, obj, NULL, Tcl_DuplicateObj(src), TCL_LEAVE_ERR_MSG);
		if (src == NULL)
			return TCL_ERROR;
	}

	/*
	fprintf(stderr, "JSON_Set, obj: \"%s\", src: \"%s\"\n",
			Tcl_GetString(obj), Tcl_GetString(src));
			*/
	target = src;

	{
		Tcl_ObjIntRep*	ir = NULL;
		TEST_OK(JSON_GetIntrepFromObj(interp, target, &type, &ir));
		val = get_unshared_val(ir);
	}

	// Walk the path as far as it exists in src
	//fprintf(stderr, "set, initial type %s\n", type_names[type]);
	for (i=0; i<pathc-1; i++) {
		step = pathv[i];
		//fprintf(stderr, "looking at step %s, cx type: %s\n", Tcl_GetString(step), type_names_int[type]);

		switch (type) {
			case JSON_UNDEF: //{{{
				THROW_ERROR("Found JSON_UNDEF type jval following path");
				//}}}
			case JSON_OBJECT: //{{{
				TEST_OK(Tcl_DictObjGet(interp, val, step, &target));
				if (target == NULL) {
					goto bad_path;
				}
				if (Tcl_IsShared(target)) {
					//fprintf(stderr, "Path element %d: \"%s\" exists but the TclObj is shared (%d), replacing it with an unshared duplicate\n",
					//		i, Tcl_GetString(step), target->refCount);
					target = Tcl_DuplicateObj(target);
					TEST_OK(Tcl_DictObjPut(interp, val, step, target));
				}
				break;
				//}}}
			case JSON_ARRAY: //{{{
				{
					int			ac, index_str_len, ok=1;
					long		index;
					const char*	index_str;
					char*		end;
					Tcl_Obj**	av;

					TEST_OK(Tcl_ListObjGetElements(interp, val, &ac, &av));
					//fprintf(stderr, "descending into array of length %d\n", ac);

					if (Tcl_GetLongFromObj(NULL, step, &index) != TCL_OK) {
						// Index isn't an integer, check for end(+/-int)?
						index_str = Tcl_GetStringFromObj(step, &index_str_len);
						if (index_str_len < 3 || strncmp("end", index_str, 3) != 0)
							ok = 0;

						if (ok) {
							index = ac-1;
							if (index_str_len >= 4) {
								if (index_str[3] != '-' && index_str[3] != '+') {
									ok = 0;
								} else {
									// errno is magically thread-safe on POSIX
									// systems (it's thread-local)
									errno = 0;
									index += strtol(index_str+3, &end, 10);
									if (errno != 0 || *end != 0)
										ok = 0;
								}
							}
						}

						if (!ok)
							THROW_ERROR("Expected an integer index or end(+/-integer)?, got ", Tcl_GetString(step));

						//fprintf(stderr, "Resolved index of %ld from \"%s\"\n", index, index_str);
					} else {
						//fprintf(stderr, "Explicit index: %ld\n", index);
					}

					if (index < 0) {
						goto bad_path;
					} else if (index >= ac) {
						goto bad_path;
					} else {
						target = av[index];
						if (Tcl_IsShared(target)) {
							target = Tcl_DuplicateObj(target);
							TEST_OK(Tcl_ListObjReplace(interp, val, index, 1, 1, &target));
						}
						//fprintf(stderr, "extracted index %ld: (%s)\n", index, Tcl_GetString(target));
					}
				}
				break;
				//}}}
			case JSON_STRING:
			case JSON_NUMBER:
			case JSON_BOOL:
			case JSON_NULL:
			case JSON_DYN_STRING:
			case JSON_DYN_NUMBER:
			case JSON_DYN_BOOL:
			case JSON_DYN_JSON:
			case JSON_DYN_TEMPLATE:
			case JSON_DYN_LITERAL:
				THROW_ERROR("Attempt to index into atomic type ", get_type_name(type), " at path key \"", Tcl_GetString(step), "\"");
				/*
				i++;
				goto bad_path;
				*/
			default:
				THROW_ERROR("Unhandled type: ", Tcl_GetString(Tcl_NewIntObj(type)));
		}

		{
			Tcl_ObjIntRep*	ir = NULL;
			TEST_OK(JSON_GetIntrepFromObj(interp, target, &type, &ir));
			val = get_unshared_val(ir);
		}
		//fprintf(stderr, "Walked on to new type %s\n", type_names[type]);
	}

	//fprintf(stderr, "Reached end of path, calling JSON_SetIntRep for replacement value %s (%s), target is %s\n",
	//		type_names_int[newtype], Tcl_GetString(replacement), type_names_int[type]);

	step = pathv[i];	// This names the key / element to unset
	//fprintf(stderr, "To replace: path step %d: \"%s\"\n", i, Tcl_GetString(step));
	switch (type) {
		case JSON_UNDEF: //{{{
			THROW_ERROR("Found JSON_UNDEF type jval following path");
			//}}}
		case JSON_OBJECT: //{{{
			TEST_OK(Tcl_DictObjRemove(interp, val, step));
			break;
			//}}}
		case JSON_ARRAY: //{{{
			{
				int			ac, index_str_len, ok=1;
				long		index;
				const char*	index_str;
				char*		end;
				Tcl_Obj**	av;

				TEST_OK(Tcl_ListObjGetElements(interp, val, &ac, &av));
				//fprintf(stderr, "descending into array of length %d\n", ac);

				if (Tcl_GetLongFromObj(NULL, step, &index) != TCL_OK) {
					// Index isn't an integer, check for end(+/-int)?
					index_str = Tcl_GetStringFromObj(step, &index_str_len);
					if (index_str_len < 3 || strncmp("end", index_str, 3) != 0)
						ok = 0;

					if (ok) {
						index = ac-1;
						if (index_str_len >= 4) {
							if (index_str[3] != '-' && index_str[3] != '+') {
								ok = 0;
							} else {
								// errno is magically thread-safe on POSIX
								// systems (it's thread-local)
								errno = 0;
								index += strtol(index_str+3, &end, 10);
								if (errno != 0 || *end != 0)
									ok = 0;
							}
						}
					}

					if (!ok)
						THROW_ERROR("Expected an integer index or end(+/-integer)?, got ", Tcl_GetString(step));

					//fprintf(stderr, "Resolved index of %ld from \"%s\"\n", index, index_str);
				} else {
					//fprintf(stderr, "Explicit index: %ld\n", index);
				}
				//fprintf(stderr, "Removing array index %d of %d\n", index, ac);

				if (index < 0) {
					break;
				} else if (index >= ac) {
					break;
				} else {
					TEST_OK(Tcl_ListObjReplace(interp, val, index, 1, 0, NULL));
					//fprintf(stderr, "extracted index %ld: (%s)\n", index, Tcl_GetString(target));
				}
			}
			break;
			//}}}
		case JSON_STRING:
		case JSON_NUMBER:
		case JSON_BOOL:
		case JSON_NULL:
		case JSON_DYN_STRING:
		case JSON_DYN_NUMBER:
		case JSON_DYN_BOOL:
		case JSON_DYN_JSON:
		case JSON_DYN_TEMPLATE:
		case JSON_DYN_LITERAL:
			{
				Tcl_Obj* bad_path = NULL;

				Tcl_IncrRefCount(bad_path = Tcl_NewListObj(i+1, pathv));
				Tcl_SetErrorCode(interp, "RL", "JSON", "BAD_PATH", Tcl_GetString(bad_path), NULL);
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Attempt to index into atomic type %s at path \"%s\"", get_type_name(type), Tcl_GetString(bad_path)));
				Tcl_DecrRefCount(bad_path); bad_path = NULL;
				return TCL_ERROR;
			}
		default:
			THROW_ERROR("Unhandled type: ", Tcl_GetString(Tcl_NewIntObj(type)));
	}

	Tcl_InvalidateStringRep(src);

	if (interp)
		Tcl_SetObjResult(interp, src);

	return TCL_OK;

bad_path:
	{
		Tcl_Obj* bad_path = NULL;

		Tcl_IncrRefCount(bad_path = Tcl_NewListObj(i+1, pathv));
		Tcl_SetErrorCode(interp, "RL", "JSON", "BAD_PATH", Tcl_GetString(bad_path), NULL);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Path element \"%s\" doesn't exist", Tcl_GetString(bad_path)));
		Tcl_DecrRefCount(bad_path); bad_path = NULL;
		return TCL_ERROR;
	}
}

//}}}
int JSON_Normalize(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* normalized)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Pretty(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj** prettyString)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Template(Tcl_Interp* interp, Tcl_Obj* template, Tcl_Obj* dict, Tcl_Obj** res) //{{{
{
	//struct interp_cx*	l = Tcl_GetAssocData(interp, "rl_json", NULL);
	Tcl_Obj*			actions = NULL;
	int					retcode = TCL_OK;
	Tcl_ObjIntRep*		ir;
	enum json_types		type;

	TEST_OK(JSON_GetIntrepFromObj(interp, template, &type, &ir));

	replace_tclobj(&actions, ir->twoPtrValue.ptr2);
	if (actions == NULL) {
		TEST_OK(build_template_actions(interp, template, &actions));
		replace_tclobj((Tcl_Obj**)&ir->twoPtrValue.ptr2, actions);
	}

	retcode = apply_template_actions(interp, template, actions, dict, res);
	release_tclobj(&actions);

	return retcode;
}

//}}}
int JSON_IsNULL(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, int* isnull)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Type(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, enum json_types* type)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Length(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, int* type)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Keys(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, Tcl_Obj** keyslist)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Decode(Tcl_Interp* interp, Tcl_Obj* obj, Tcl_Obj* path, Tcl_Obj** keys)
{
	THROW_ERROR("Not implemented yet");
}

int JSON_Foreach(Tcl_Interp* interp, Tcl_Obj* iterators, int* body, enum collecting_mode collect, Tcl_Obj** res, ClientData cdata)
{
	THROW_ERROR("Not implemented yet");
}

/* Local Variables: */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* End: */
// vim: foldmethod=marker foldmarker={{{,}}} ts=4 shiftwidth=4

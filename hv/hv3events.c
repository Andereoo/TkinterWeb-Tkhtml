/*----------------------------------------------------------------------- 
 * DOM Level 2 Events for Hv3.
 *
 *     // Introduced in DOM Level 2:
 *     interface EventTarget {
 *         void addEventListener(
 *             in DOMString type, 
 *             in EventListener listener, 
 *             in boolean useCapture
 *         );
 *         void removeEventListener(
 *             in DOMString type, 
 *             in EventListener listener, 
 *             in boolean useCapture
 *         );
 *         boolean dispatchEvent(in Event evt) raises(EventException);
 *     };
 */

/*
 * By defining STOP_PROPAGATION as "cancelBubble", we can also support the mozilla extension Event.
 * cancelBubble. Setting it to true "cancels bubbling", just like calling stopPropagation().
 */
#define STOP_PROPAGATION        "cancelBubble"
#define PREVENT_DEFAULT         "hv3__qjs__preventDefault"
#define CALLED_LISTENER         "hv3__qjs__calledListener"

#define CFUNCTION(ctx, o, name, func, len) JS_SetPropertyStr(ctx, o, name, JS_NewCFunction(ctx, func, name, len))

typedef struct EventTarget EventTarget;
typedef struct ListenerContainer ListenerContainer;

struct EventTarget {  // Alas, you are now a relic from a bygone age
	QjsInterp *pQjs;
	EventType *pTypeList;
};

struct EventType {
	JSAtom zType;
	ListenerContainer *pListenerList;
	EventType *pNext;
};

struct ListenerContainer {
	uint8_t isCapture;         /* True if a capturing event */
	JSValue listener;          /* Listener function */
	ListenerContainer *pNext;  /* Next listener on this event type */
};

static inline uint8_t valueToBoolean(JSContext *ctx, JSValue val, uint8_t eDef)
{
    switch (JS_VALUE_GET_TAG(val)) {
        case JS_TAG_INT:
        case JS_TAG_BOOL:
        case JS_TAG_FLOAT64:
            return JS_ToBool(ctx, val);
    }
    return eDef;
}

/* Helper function to convert value to boolean */
static inline void setBooleanFlag(JSContext *ctx, JSValue obj, const char *z, int v)
{
	JS_SetPropertyStr(ctx, obj, z, JS_NewBool(ctx, v));
}

/* Helper function to set boolean flag */
static inline int getBooleanFlag(JSContext *ctx, JSValue obj, const char *z)
{
    return valueToBoolean(ctx, JS_GetPropertyStr(ctx, obj, z), 0);
}

static EventType **getEventList(JSValue obj) {
    JSClassID id = JS_GetClassID(obj);
	if (id == QjsTclClassId || id == QjsTclCallClassId) {
        return &((QjsTclObject*)JS_GetOpaque(obj, id))->pTypeList;
    }
    return NULL;
}

/* Run event handler */
static uint8_t
runEvent(JSContext *ctx, JSValue target, JSValue event, JSValue zType, uint8_t isCapture)
{
    int rc = 1;
    EventType *pET, **apET;
	ListenerContainer *pL;

    assert(JS_IsObject(event));

    /* Assert that zType is a string and isCapture is boolean */
    if (!JS_IsString(zType) || isCapture > 1) {
        JS_ThrowTypeError(ctx, "Invalid zType or isCapture");
        return 0;
    }
    /* Set event.currentTarget = target */
    JS_SetPropertyStr(ctx, event, "currentTarget", target);

    /* Check if stopPropagation() has been called */
    if (getBooleanFlag(ctx, event, STOP_PROPAGATION)) {
        return 0;
    }

	apET = getEventList(target);
    /* If this is a Tcl based object, run any requested registered DOM event handlers */
    if (apET) {
		JSAtom a = JS_ValueToAtom(ctx, zType);
        for (pET = *apET; pET && pET->zType != a; pET = pET->pNext);
		if (pET) {
			for (pL = pET->pListenerList; rc && pL; pL = pL->pNext) {
				if (pL->isCapture == isCapture) {
					JS_Call(ctx, pL->listener, target, 1, &event);
					setBooleanFlag(ctx, event, CALLED_LISTENER, 1);
				}
				if (pET->pListenerList != pL) break;
			}
		}
		JS_FreeAtom(ctx, a);
    }
    /* If this is not the "capturing" phase, run the legacy event-handler. */
    if (!isCapture) {
		JSValue e = JS_ConcatString(ctx, JS_NewString(ctx, "on"), JS_DupValue(ctx, zType));
		JSAtom a = JS_ValueToAtom(ctx, e);

		JSValue res = JS_Invoke(ctx, target, a, 1, &event);
        setBooleanFlag(ctx, event, CALLED_LISTENER, 1);
        rc = valueToBoolean(ctx, res, 1);
		JS_FreeValue(ctx, e);
		JS_FreeAtom(ctx, a);

        if (rc < 1) {
            setBooleanFlag(ctx, event, PREVENT_DEFAULT, 1);
            setBooleanFlag(ctx, event, STOP_PROPAGATION, 1);
        }
		JS_FreeValue(ctx, res);
    }
    return rc;
}

/* preventDefault function */
static JSValue preventDefaultFunc(JSContext *ctx, JSValueConst this, int c, JSValueConst *v)
{
    setBooleanFlag(ctx, this, PREVENT_DEFAULT, 1);
}

/* stopPropagation function */
static JSValue stopPropagationFunc(JSContext *ctx, JSValueConst this, int c, JSValueConst *v)
{
    setBooleanFlag(ctx, this, STOP_PROPAGATION, 1);
}

static JSValue getParentNode(JSContext *ctx, JSValue o)
{
    if (JS_GetClassID(o) == QjsTclClassId) {
        NodeHack *pNode = ((QjsTclObject*)JS_GetOpaque(o, QjsTclClassId))->nodehandle;
        if (pNode && pNode->pParent && pNode->pParent->pNodeObj){
            return *pNode->pParent->pNodeObj;
        }
        if (pNode && pNode->pParent == NULL && pNode->iNode < 0) {
            return JS_NULL;
        }
        if (pNode && pNode->pParent == NULL) {
            /* Return document... */
        }
    }
    JSValue val = JS_GetPropertyStr(ctx, o, "parentNode");
    return JS_IsUndefined(val) ? JS_NULL : val;
}

/*
 *---------------------------------------------------------------------------
 *
 * dispatchEventFunc --
 *
 *     Implementation of DOM method EventTarget.dispatchEvent().
 *
 *     According to the DOM, the boolean return value of dispatchEvent()
 *     indicates whether any of the listeners which handled the event 
 *     called preventDefault. If preventDefault was called the value is 
 *     false, else the value is true.
 *
 *     Before running any event-handlers, the following are added to
 *     the event object passed as the only argument:
 *
 *         target
 *         stopPropagation()
 *         preventDefault()
 *
 *         currentTarget            (updated throughout)
 *         eventPhase               (updated throughout)
 *
 * Results: 
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static JSValue dispatchEventFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    unsigned int nNodesAlloc = 0, nNodes = 0;
	int8_t isRun = 1;
    JSValue zType, event, *apNodes = NULL;

    /* Check the number of function arguments. */
    if (argc != 1) {
        return JS_ThrowTypeError(ctx, "Function requires exactly 1 parameter");
    }

    if (!JS_IsObject(argv[0]) || JS_GetClassID(argv[0])>=QjsTclClassId) 
		return JS_ThrowTypeError(ctx, "Function parameter must be 'native' object");

    event = argv[0];

	CFUNCTION(ctx, event, "stopPropagation", stopPropagationFunc, 0);
	CFUNCTION(ctx, event, "preventDefault", preventDefaultFunc, 0);

	JS_SetPropertyStr(ctx, event, "target", JS_DupValue(ctx, this)); // Equivalent to SEE_OBJECT_PUTA

    setBooleanFlag(ctx, event, STOP_PROPAGATION, 0);
    setBooleanFlag(ctx, event, PREVENT_DEFAULT, 0);
    setBooleanFlag(ctx, event, CALLED_LISTENER, 0);

    /* Get the event type */
	zType = JS_GetPropertyStr(ctx, event, "type");
    if (!JS_IsString(zType)) {
        /* Event without a type - matches no listeners */
		JS_FreeValue(ctx, zType);
        return JS_ThrowTypeError(ctx, "UNSPECIFIED_EVENT_TYPE_ERR");
    }

    /* If this is a bubbling event, create a list of the nodes ancestry
     * to deliver it to now. This is because any callbacks that modify
     * the document tree are not supposed to affect the delivery of
     * this event. */
	if (valueToBoolean(ctx, JS_GetPropertyStr(ctx, event, "bubbles"), 0)) {
        JSValue node = this;
        do {
            node = getParentNode(ctx, node);
            if (nNodes == nNodesAlloc) {
                nNodesAlloc = 2 * (1 + nNodesAlloc); // Use an exponential growth algorithm to minimize reallocations
                apNodes = js_realloc(ctx, apNodes, sizeof(JSValue) * nNodesAlloc);
            }
            apNodes[nNodes++] = JS_DupValue(ctx, node);
        } while (!JS_IsNull(node));
    }

    /* Deliver the "capturing" phase of the event. */
    JS_SetPropertyStr(ctx, event, "eventPhase", JS_NewInt32(ctx, 1));
    for (int i = nNodes - 1; isRun && i >= 0; i--) {
        isRun = runEvent(ctx, apNodes[i], event, zType, 1);
    }

    /* Deliver the "target" phase of the event. */
    JS_SetPropertyStr(ctx, event, "eventPhase", JS_NewInt32(ctx, 2));
    if (isRun) isRun = runEvent(ctx, this, event, zType, 0);

    /* Deliver the "bubbling" phase of the event. */
    JS_SetPropertyStr(ctx, event, "eventPhase", JS_NewInt32(ctx, 3));
    for (int i = 0; isRun && i < nNodes; i++) {
        isRun = runEvent(ctx, apNodes[i], event, zType, 0);
    }

	JS_FreeValue(ctx, zType);
	if (apNodes && nNodesAlloc) js_free(ctx, apNodes);

    /* Set the return value. */
    return JS_NewBool(ctx, getBooleanFlag(ctx, event, PREVENT_DEFAULT));
}

/*
 *---------------------------------------------------------------------------
 *
 * eventDispatchCmd --
 *
 *     $qjs dispatch TARGET-COMMAND EVENT-COMMAND
 *
 * Results: 
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static int
eventDispatchCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
	QjsInterp *qjs = (QjsInterp *)cd;
    JSValue target, event, ret;
    int rc = TCL_OK;

    target = findOrCreateObject(qjs, objv[2]);
    event = createNative(qjs, objv[3]);

    assert(Tcl_IsShared(objv[3]));

    ret = dispatchEventFunc(qjs->ctx, target, 1, &event);

    if (JS_IsException(ret)) {
        rc = handleJavascriptError(qjs);
    } else {
        int isHandled = getBooleanFlag(qjs->ctx, event, CALLED_LISTENER);
        int isPrevent = getBooleanFlag(qjs->ctx, event, PREVENT_DEFAULT);
        Tcl_Obj *pRet = Tcl_NewObj();
        Tcl_ListObjAppendElement(interp, pRet, Tcl_NewBooleanObj(isHandled));
        Tcl_ListObjAppendElement(interp, pRet, Tcl_NewBooleanObj(isPrevent));
        Tcl_SetObjResult(interp, pRet);
    }
	JS_FreeValue(qjs->ctx, event);
	JS_FreeValue(qjs->ctx, ret);
    return rc;
}

/*
 *---------------------------------------------------------------------------
 *
 * addEventListenerFunc --
 *
 *     Implementation of DOM method EventTarget.addEventListener():
 *
 *         void addEventListener(
 *             in DOMString type, 
 *             in EventListener listener, 
 *             in boolean useCapture
 *         );
 *
 * Results: 
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static JSValue
addEventListenerFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    EventType *pET, **apET;
    ListenerContainer *pL;
    /* Parameters passed to javascript function */
    uint8_t useCapture = 0;

    /* Check the number of function arguments. */
    if (argc != 2 && argc != 3) {
        return JS_ThrowTypeError(ctx, "1-2 arguments required, but non present.");
    }

    /* Check that 'this' is a Tcl based object */
    apET = getEventList(this);
    if (!apET) return JS_ThrowTypeError(ctx, "Bad type for 'this'");

	if (argc > 2) useCapture = JS_ToBool(ctx, argv[2]);  /* Parse the arguments */
	JSAtom zType = JS_ValueToAtom(ctx, argv[0]);

    for (pET = *apET; pET && pET->zType != zType; pET = pET->pNext);
    if (!pET) {
        pET = js_mallocz(ctx, sizeof(*pET));
        pET->zType = JS_DupAtom(ctx, zType);
        pET->pNext = *apET;
        *apET = pET;
    }
	JS_FreeAtom(ctx, zType);

    /* Check that this is not an attempt to insert a duplicate 
     * event-listener. From the DOM Level 2 spec:
     *
     *     "If multiple identical EventListeners are registered on the same
     *     EventTarget with the same parameters the duplicate instances are
     *     discarded."
     */
    for (
        pL = pET->pListenerList; 
        pL && (pL->isCapture != useCapture || !JS_StrictEq(ctx, pL->listener, argv[1])); 
        pL = pL->pNext
    );
    if (pL) return JS_UNDEFINED;

    pL = js_mallocz(ctx, sizeof(*pL));
    pL->pNext = pET->pListenerList;
    pET->pListenerList = pL;
    pL->isCapture = useCapture;
    pL->listener = JS_DupValue(ctx, argv[1]);
    /* DOM says return value is "void" */
    return JS_UNDEFINED;
}

/*
 *---------------------------------------------------------------------------
 *
 * removeEventListenerFunc --
 *
 *     Implementation of DOM method EventTarget.removeEventListener():
 *
 *         void removeEventListener(
 *             in DOMString type, 
 *             in EventListener listener, 
 *             in boolean useCapture
 *         );
 * Results: 
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static JSValue
removeEventListenerFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    EventType *pET, **apET;
    uint8_t useCapture = 0;

    /* Check the number of function arguments. */
    if (argc != 2 && argc != 3) {
		return JS_ThrowTypeError(ctx, "2-3 arguments required, but non present.");
    }
    /* Check that this is a Tcl based object. */
    apET = getEventList(this);
    if (!apET) {
        return JS_ThrowTypeError(ctx, "Bad type for 'this'");
    }
    if (argc > 2) useCapture = valueToBoolean(ctx, argv[2], 0);  /* Parse the arguments */

	JSAtom zType = JS_ValueToAtom(ctx, argv[0]);
    for (pET = *apET; pET && pET->zType != zType; pET = pET->pNext);
    if (pET) {
        ListenerContainer *pL, **apL = &pET->pListenerList;
        for (pL = *apL; pL; pL = pL->pNext) {
            if (pL->isCapture == useCapture && JS_StrictEq(ctx, pL->listener, argv[1])) {
                *apL = pL->pNext;
                JS_FreeValue(ctx, pL->listener);
                js_free(ctx, pL);
                break;
            } else {
                apL = &pL->pNext;
            }
        }
    }
	JS_FreeAtom(ctx, zType);
    /* DOM says return value is "void" */
    return JS_UNDEFINED;
}

static JSValue EventFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
	/* Check the number of function arguments. */
    if (argc > 2 || argc < 1) {
		return JS_ThrowTypeError(ctx, "1-2 arguments required, but non present.");
    }
	JSValue opt, event = JS_NewObject(ctx);
	JS_SetPropertyStr(ctx, event, "type", JS_DupValue(ctx, argv[0]));
	if (argc > 1) {
		opt = JS_GetPropertyStr(ctx, argv[1], "bubbles");
		if (!JS_IsUndefined(opt)) JS_SetPropertyStr(ctx, event, "bubbles", opt);
		opt = JS_GetPropertyStr(ctx, argv[1], "cancelable");
		if (!JS_IsUndefined(opt)) JS_SetPropertyStr(ctx, event, "cancelable", opt);
	}
	JS_DefinePropertyValue(ctx, event, JS_ATOM_Symbol_toStringTag, JS_NewString(ctx, "Event"), 0);
	return event;
}

/*
 *---------------------------------------------------------------------------
 *
 * eventTargetInit --
 *
 *     This function initialises the events sub-system for the
 *     QjsTclObject passed as an argument. In practice, this means
 *     it evaluates the Tcl script:
 *
 *         eval $obj Events
 *
 *     where $obj is the Tcl command implementing the object. The
 *     return value is expected to be a list of alternating attribute 
 *     names and values. Each value is compiled to a javascript function
 *     and inserted into QjsTclObject.pNative using the supplied attribute
 *     name. For example, if the [Events] script returns:
 *
 *         onclick {alert("click!"} ondblclick {alert("dblclick!")}
 *  
 *     The "onclick" and "ondblclick" properties of QjsTclObject
 *     are set to the following objects, respectively:
 *
 *         function (event) { alert("click!") }
 *         function (event) { alert("dblclick!") }
 *
 * Results: 
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static void eventTargetInit(QjsInterp *qjs, JSValue o)
{
    Tcl_Interp *pTcl = qjs->interp;
    Tcl_Obj *pList, **apWord;
    int nWord, i, l;

    if (callQjsTclMethod(pTcl, NULL, o, Tcl_NewStringObj("Events", 6), NULL) != TCL_OK) {
        Tcl_BackgroundError(pTcl);
        return;
    }
    pList = Tcl_GetObjResult(pTcl);
    if (Tcl_ListObjGetElements(pTcl, pList, &nWord, &apWord) != TCL_OK) {
        Tcl_BackgroundError(pTcl);
        return;
    }
    for (i = 0; i < nWord-1; i += 2){
        /* Construct a string like this:
         *
         *   this.$zAttr = function (event) { $zScript }
         *
         * We then evaluate the script with the "this" object set to the
         * object we are trying to attach the legacy event handler to.
         */
        Tcl_Obj *pJ = Tcl_NewStringObj("this.", 5);
        Tcl_IncrRefCount(pJ);
        Tcl_AppendObjToObj(pJ, apWord[i]);
        Tcl_AppendToObj(pJ, " = function (event) {", 21);
        Tcl_AppendObjToObj(pJ, apWord[i+1]);
        Tcl_AppendToObj(pJ, "}", 1);
        /* printf("%s\n", Tcl_GetString(pJ)); */

		const char *pJz = Tcl_GetStringFromObj(pJ, &l);
        JSValue res = JS_EvalThis(qjs->ctx, o, pJz, l, "<event>", JS_EVAL_TYPE_GLOBAL);
        /* Not a lot we can do with an error here... */
        JS_FreeValue(qjs->ctx, res);
        Tcl_DecrRefCount(pJ);
    }
    Tcl_ResetResult(pTcl);
}

/*
 *---------------------------------------------------------------------------
 *
 * eventTargetGlobalInit --
 *
 *     This function initialises the events sub-system for the TclObject passed as an argument.
 *
 *     Add entries to JSObject for the following built-in object methods (DOM Interface EventTarget):
 *
 *         dispatchEvent()
 *         addEventListener()
 *         removeEventListener()
 *
 * Results: 
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static void eventTargetGlobalInit(QjsInterp *qjs, JSValue g)
{
    JSValue proto = JS_NewObject(qjs->ctx);
	static const JSCFunctionListEntry funcs[] = {
		JS_CFUNC_DEF("dispatchEvent",       1, dispatchEventFunc),
		JS_CFUNC_DEF("removeEventListener", 3, removeEventListenerFunc),
		JS_CFUNC_DEF("addEventListener",    3, addEventListenerFunc),
	};
    JS_SetPropertyFunctionList(qjs->ctx, proto, funcs, 3);
	JS_SetClassProto(qjs->ctx, QjsTclClassId, proto);
	JS_SetClassProto(qjs->ctx, QjsTclCallClassId, JS_DupValue(qjs->ctx, proto));
	JS_SetPropertyStr(qjs->ctx, g, "Event", JS_NewCFunction2(qjs->ctx, EventFunc, "Event", 2, JS_CFUNC_constructor, 0));
}

static void listenerMark(JSRuntime *rt, JSValueConst obj, JS_MarkFunc *mark_func)
{
    QjsTclObject *p = (QjsTclObject*)JS_GetOpaque(obj, QjsTclClassId);
	if (p == NULL) p = (QjsTclObject*)JS_GetOpaque(obj, QjsTclCallClassId);
    for(EventType *pET = p->pTypeList; pET; pET = pET->pNext) {
		for (ListenerContainer *pL = pET->pListenerList; pL; pL = pL->pNext) {
			JS_MarkValue(rt, pL->listener, mark_func);
		}
    }
}

static void freeEventTargetData(JSRuntime *rt, QjsTclObject *pTclObject)
{
    EventType *pET, *pETNext;
    ListenerContainer *pL, *pLNext;
    /* Start with the EventType list */
    for (pET = pTclObject->pTypeList; pET; pET = pETNext) {
        /* Save the next EventType pointer before freeing */
        pETNext = pET->pNext;
        /* Free the ListenerContainer list for this EventType */
        for (pL = pET->pListenerList; pL; pL = pLNext) {
            pLNext = pL->pNext;  /* Save the next ListenerContainer pointer before freeing */
			JS_FreeValueRT(rt, pL->listener);
            js_free_rt(rt, pL);  /* Free the ListenerContainer structure */
        }
        /* Free the JSValue for the event type */
        JS_FreeAtomRT(rt, pET->zType);
        /* Free the EventType structure */
        js_free_rt(rt, pET);
    }
    pTclObject->pTypeList = NULL;  /* Clear the pTypeList pointer */
}

/*
 *---------------------------------------------------------------------------
 *
 * eventTargetDump --
 *
 *         $qjs events TCL-COMMAND
 *
 *     This function is used to introspect event-listeners from
 *     the Tcl level. The return value is a list. Each element of
 *     the list takes the following form:
 *
 *       {EVENT-TYPE LISTENER-TYPE JAVASCRIPT}
 *
 *     where EVENT-TYPE is the event-type string passed to [addEventListener]
 *     or [setLegacyListener]. LISTENER-TYPE is one of "legacy", "capturing"
 *     or "non-capturing". JAVASCRIPT is the "tostring" version of the
 *     js object to call to process the event.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static int
eventDumpCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    QjsInterp *qjs = (QjsInterp*)cd;
    struct JSContext *ctx = qjs->ctx;
    EventType *pType;
    Tcl_Obj *apRow[3], *pRet;

    uint32_t l, i;
    JSPropertyEnum *pEnum;
    JSValue prop;

    assert(objc == 3);
    JSValue obj = findOrCreateObject(qjs, objv[2]);

    pRet = Tcl_NewObj();
    Tcl_IncrRefCount(pRet);

    for (pType = *getEventList(obj); pType; pType = pType->pNext) {
        ListenerContainer *pL;
        apRow[0] = atomToObj(ctx, pType->zType);

        for (pL = pType->pListenerList; pL; pL = pL->pNext) {
            const char *z = (pL->isCapture ? "capturing" : "non-capturing");
            apRow[1] = Tcl_NewStringObj(z, -1);
            apRow[2] = stringToObj(ctx, pL->listener);
            Tcl_ListObjAppendElement(interp, pRet, Tcl_NewListObj(3, apRow));
        }
    }

	JS_GetOwnPropertyNames(ctx, &pEnum, &l, obj, JS_GPN_STRING_MASK);
	for (i = 0; i < l; i++) {
        const char *zProp = JS_AtomToCString(ctx, pEnum[i].atom);
        if (strncmp(zProp, "on", 2) == 0) {
            JSValue val = JS_GetProperty(ctx, obj, pEnum[i].atom);
            if (JS_IsObject(val)) {
                apRow[0] = Tcl_NewStringObj(&zProp[2], -1);
                apRow[1] = Tcl_NewStringObj("legacy", 6);
                apRow[2] = stringToObj(ctx, val);
                Tcl_ListObjAppendElement(interp, pRet, Tcl_NewListObj(3, apRow));
            }
			JS_FreeValue(ctx, val);
        }
		JS_FreeCString(ctx, zProp);
	}
	JS_FreePropertyEnum(ctx, pEnum, l);
    Tcl_SetObjResult(interp, pRet);
    Tcl_DecrRefCount(pRet);
	JS_FreeValue(ctx, obj);
    return TCL_OK;
}
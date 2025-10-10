/*
 * hv3timeout.c --
 *
 *     This file contains C-code that implements the following 
 *     methods of the global object in the Hv3 web browser:
 *
 *         setTimeout() 
 *         clearTimeout() 
 *         setInterval() 
 *         clearInterval() 
 *
 *     Events are scheduled using the Tcl event loop (specifically the
 *     Tcl_CreateTimerHandler() API).
 *
 *
 * TODO: Copyright.
 */

#define NO_INTERVAL (-1)
struct QjsTimeout {
	Tcl_TimerToken token;
	JSContext *ctx;
	JSValue func;
	int nArg;
	JSValue *apArg;
	int interval;  /* Number of milliseconds for setInterval(). Or -1 for setTimeout(). */
	int id;  /* Linked list pointers and id number. */
	QjsTimeout *pNext, **ppThis;
};

static void delTimeout(QjsTimeout *p) {
	if (p->token) return;
	*p->ppThis = p->pNext;
    if (p->pNext) p->pNext->ppThis = p->ppThis;
    JS_FreeValue(p->ctx, p->func);
	for (int i=0; i < p->nArg; i++) JS_FreeValue(p->ctx, p->apArg[i]);
	if (p->nArg > 0) js_free(p->ctx, p->apArg);
	js_free(p->ctx, p);
}

static void timeoutCb(ClientData clientData) {
    QjsTimeout *p = (QjsTimeout *)clientData;
    JSValue res;
	
    assert(p->ppThis);

    if (JS_IsFunction(p->ctx, p->func)) {
		JSValue glb = JS_GetGlobalObject(p->ctx);
        res = JS_Call(p->ctx, p->func, glb, p->nArg, p->apArg);
		JS_FreeValue(p->ctx, glb);
    }
	JS_FreeValue(p->ctx, res);
    if (p->token) {
        /* If p->token was NULL, then the callback has invoked javascript
         * function cancelInterval() or cancelTimeout(). Otherwise, either
         * reschedule the callback (for an interval) or remove the structure
         * from the linked-list so the garbage-collector can find it (for a 
         * timeout).
         */
        if (p->interval <= NO_INTERVAL) {
            delTimeout(p);
        } else if (p->token) {
            ClientData c = (ClientData)p;
            assert(p->ppThis);
            p->token = Tcl_CreateTimerHandler(p->interval, timeoutCb, c);
        }
    }
}

static JSValue newTimer(
    JSContext *ctx, JSValueConst this, uint8_t isInterval, int argc, JSValueConst *argv
) {
    QjsTimeout *p;
    uint64_t milli;
    /* Check the number of function arguments. */
    if (isInterval && argc < 2) return JS_ThrowTypeError(ctx, "Function requires at least 2 parameters");
    else if (argc < 1) return JS_ThrowTypeError(ctx, "Function requires at least 1 parameter");
    /* Check that if there are more than 2 arguments, the first argument is an object, not a string. */
    if (!JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "First argument must be of type object");
    }
    /* Convert the second argument to an integer (number of milliseconds) */
	if (argc > 1) {
		if (JS_NumberIsNegativeOrMinusZero(ctx, argv[1]) || JS_ToInt64(ctx, &milli, argv[1])) return JS_EXCEPTION;
	} else milli = 0;

    /* Allocate the new QjsTimeout structure and populate the 
     * SeeTimout.func and QjsTimeout.apArg variables.
     */
    p = js_malloc(ctx, sizeof(*p));
    p->func = JS_DupValue(ctx, argv[0]);
	
	const int OFF = 1 + JS_IsNumber(argv[1]);
	if (argc > OFF) {
		p->nArg = argc - OFF;
		p->apArg = js_malloc(ctx, sizeof(JSValue)*p->nArg);
		for (int i=0; i < p->nArg; i++) p->apArg[i] = JS_DupValue(ctx, argv[i+OFF]);
	}
	
	if (isInterval && milli < 10) milli = 10;

	ContextOpaque* co = JS_GetContextOpaque(ctx);
    p->interval = (isInterval ? milli : NO_INTERVAL);
    p->ctx = ctx;
    p->id = co->iNextTimeout++;
    p->pNext = co->pTimeout;
    co->pTimeout = p;
	p->ppThis = &co->pTimeout;
    if (p->pNext) p->pNext->ppThis = &p->pNext;
    assert(p->ppThis);

    p->token = Tcl_CreateTimerHandler(milli, timeoutCb, (ClientData)p);
    return JS_NewUint32(ctx, p->id);
}

static JSValue cancelTimer(
    JSContext *ctx, JSValueConst this, uint8_t isInterval, int argc, JSValueConst *argv
) {
    uint32_t id;
    QjsTimeout *p;
    ContextOpaque* co = JS_GetContextOpaque(ctx);

    /* Check the number of function arguments. */
    if (argc != 1) {
        return JS_ThrowTypeError(ctx, "Function requires exactly 1 parameter");
    }
    if (JS_ToInt32(ctx, &id, argv[0])) return JS_EXCEPTION;

    for (p = co->pTimeout; p; p = p->pNext) {
        if (p->id == id) {
			Tcl_DeleteTimerHandler(p->token);
			p->token = NULL;
            delTimeout(p);
            break;
        }
    }
}

static JSValue setTimeoutFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    return newTimer(ctx, this, 0, argc, argv);
}
static JSValue setIntervalFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    return newTimer(ctx, this, 1, argc, argv);
}
static JSValue clearTimeoutFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    cancelTimer(ctx, this, 0, argc, argv);
}
static JSValue clearIntervalFunc(JSContext *ctx, JSValueConst this, int argc, JSValueConst *argv)
{
    cancelTimer(ctx, this, 1, argc, argv);
}

static void interpTimeoutInit(JSContext *ctx) {
	JSValue g = JS_GetGlobalObject(ctx);
	static const JSCFunctionListEntry funcs[] = {
		JS_CFUNC_DEF("setTimeout", 1, setTimeoutFunc),
		JS_CFUNC_DEF("setInterval", 2, setIntervalFunc),
		JS_CFUNC_DEF("clearTimeout", 1, clearTimeoutFunc),
		JS_CFUNC_DEF("clearInterval", 1, clearIntervalFunc),
	};
    JS_SetPropertyFunctionList(ctx, g, funcs, 4);
	JS_FreeValue(ctx, g);
}

static void interpTimeoutCleanup(QjsInterp *qjs) {
    for (QjsTimeout *q, *p = qjs->pTimeout; p; p = q) {
		q = p->pNext;
		Tcl_DeleteTimerHandler(p->token);
		p->token = NULL;
		delTimeout(p);
	}
    qjs->pTimeout = NULL; // Reset the list head
}


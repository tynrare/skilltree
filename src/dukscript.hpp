#include "external/duktape.h"
#include "raylib.h"

static duk_ret_t native_print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	TraceLog(LOG_INFO, TextFormat("%s", duk_safe_to_string(ctx, -1)));
	return 0;
}

static duk_ret_t native_sum(duk_context *ctx) {
	int i;
	int n = duk_get_top(ctx);  /* #args */
	double res = 0.0;

	for (i = 0; i < n; i++) {
		res += duk_to_number(ctx, i);
	}

	duk_push_number(ctx, res);
	return 1;  /* one return value */
}

class Dukscript {
	duk_context *ctx;

	void init(duk_context *ctx) {
		duk_push_c_function(ctx, native_print, DUK_VARARGS);
		duk_put_global_string(ctx, "print");
		duk_push_c_function(ctx, native_sum, DUK_VARARGS);
		duk_put_global_string(ctx, "sum");
	}

	public:

	Dukscript() {
		this->ctx = duk_create_heap_default();
		this->init(this->ctx);
	}
	
	~Dukscript() {
		duk_destroy_heap(this->ctx);
	}

	void eval(const char *script) {
		duk_eval_string(this->ctx, script);
	}
	
};

#include "external/duktape.h"
#include "raylib.h"

static duk_ret_t native_print(duk_context *ctx) {
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	TraceLog(LOG_INFO, TextFormat("%s", duk_safe_to_string(ctx, -1)));
	return 0;
}

static duk_ret_t call_json_decode(duk_context *ctx, void *t) {
	duk_json_decode(ctx, -1);

	return 1;
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
		duk_pop(this->ctx); // ignore result
	}


	/**
	 * evals json. Work with stack manually after that
	 * @param filename path to file
	 * @returns {bool} true if parsing success
	 */
	bool parse_json_file(const char *filename) {
		const char *filecontent = LoadFileText(filename);
		if (filecontent == NULL) {
			return false;
		}
		duk_push_string(this->ctx, filecontent);
		int rc = duk_safe_call(this->ctx, call_json_decode, NULL, 1, 1);
		if (rc != DUK_EXEC_SUCCESS) {
			TraceLog(LOG_ERROR, TextFormat("Error parsing JSON: %s\n", duk_safe_to_string(ctx, -1)));

			return false;
		}

		duk_enum(this->ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);

		return true;
	}

	/**
	 * Call pop(2) after calling next(true)
	 *
	 * @param with_value
	 *
	 * @return 
	 */
	int next(bool with_value = true) {
		return duk_next(this->ctx, -1, with_value ? 1 : 0);
	}

	const char *get_string(int idx = -1) {
		return duk_get_string(this->ctx, idx);
	}

	/**
	 * Do it when you need "enter" nested object
	 * Don't forget to pop() after all operations made
	 *
	 * @param key
	 * @param idx
	 */
	bool read_object(const char *key, int idx = -1) {
		bool rc = duk_get_prop_string(this->ctx, idx, key);

		return rc;
	}

	bool read_object(int idx = -1) {
		return duk_get_prop(this->ctx, idx);
	}

	int get_array_length(int idx = -1) {
		return duk_get_length(this->ctx, idx);
	}

	const char *get_string_by_key(const char *key, const char *def, int idx = -1) {
		duk_get_prop_string(this->ctx, idx, key);
		const char *str = def;
		if (duk_is_string(this->ctx, -1)) {
			str = duk_to_string(this->ctx, -1);
		}
		this->pop();

		return str;
	}

	int get_int_by_key(const char *key, int def,int idx = -1) {
		duk_get_prop_string(this->ctx, idx, key);
		int v = def;
		if (duk_is_number(this->ctx, -1)) {
			v = duk_to_number(this->ctx, -1);
		}
		this->pop();

		return v;
	}

	bool get_bool_by_key(const char *key, bool def,int idx = -1) {
		duk_get_prop_string(this->ctx, idx, key);
		bool v = def;
		if (duk_is_boolean(this->ctx, -1)) {
			v = duk_to_boolean(this->ctx, -1);
		}
		this->pop();

		return v;
	}

	const char *get_string_by_index(int index, const char *def, int idx = -1) {
		duk_get_prop_index(this->ctx, idx, index);
		const char *str = def;
		if (duk_is_string(this->ctx, -1)) {
			str = duk_to_string(this->ctx, -1);
		}
		this->pop();

		return str;
	}

	int get_int_by_index(int index, int def,int idx = -1) {
		duk_get_prop_index(this->ctx, idx, index);
		int v = def;
		if (duk_is_number(this->ctx, -1)) {
			v = duk_to_number(this->ctx, -1);
		}
		this->pop();

		return v;
	}

	void pop(int count = 1) {
		duk_pop_n(this->ctx, count);
	}

	private:
	
};

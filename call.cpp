#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <string>
#include <dlfcn.h>

using std::cin, std::cout, std::cerr, std::endl, std::string,
        std::vector, std::stringstream;

enum
{
    INT_TYPE,
    FLOAT_TYPE
};

struct ValueAndType
{
    unsigned long long value;
    long long type = INT_TYPE;
    char* buffer_for_string = nullptr;
};

int
assembly_call(unsigned long long* args)
{
    unsigned long long rax_value = 0;
    double xmm0_value;
    cout << "============ CALLING FUNCTION ============" << endl;
    asm volatile(
        "movsd -136(%%rax), %%xmm0;"
        "movsd -120(%%rax), %%xmm1;"
        "movsd -104(%%rax), %%xmm2;"
        "movsd -88(%%rax), %%xmm3;"
        "movsd -72(%%rax), %%xmm4;"
        "movsd -56(%%rax), %%xmm5;"
        "movsd -40(%%rax), %%xmm6;"
        "movsd -24(%%rax), %%xmm7;"
        "mov $0, %%rdi;"
        "for_begin_label:"
            "cmpq %%rdi, (%%rax);"
                "jna for_end_label;"
                    "pushq 64(%%rax, %%rdi, 8);"
                    "add $1, %%rdi;"
            "jmp for_begin_label;"
        "for_end_label:"
        "mov 16(%%rax), %%rdi;"
        "movq 24(%%rax), %%rsi;"
        "movq 32(%%rax), %%rdx;"
        "movq 40(%%rax), %%rcx;"
        "movq 48(%%rax), %%r8;"
        "movq 56(%%rax), %%r9;"
        "movq %%rax, %%rbx;"
        "addq $8, %%rbx;"
        "mov -8(%%rax), %%rax;"
        "call *(%%rbx);"
        "subq $8, %%rbx;"
        "movq (%%rbx), %%rbx;"
        "shlq $3, %%rbx;"
        "add %%rbx, %%rsp;"
        "movq %%xmm0, %%rbx"
        : "=a"(rax_value), "=b"(xmm0_value)
        : "a"(args)
        : 
    );
    cout << endl;
    cout << "========== END OF FUNCTION CALL ==========" << endl;
    cout << "%rax value = "  << rax_value << endl;
    cout << "%xmm0 value = " << xmm0_value << endl;
    return 0;
}

/*
vector of args converter to values of
[%xmm0..%xmm7, number of floating point arguments, number of arguments on stack,
    func_address, %rdi, %rsi, %rdx, %rcx, %r8, %r9 and function arguments on stack]        
*/
unsigned long long*
argswt2args(unsigned long long func_address, const vector<ValueAndType>& args_with_types)
{
    size_t len = args_with_types.size();

    int int_type_counter = 0;
    int float_type_counter = 0;
    for (int i = 0; i < len; i++) {
        if (args_with_types[i].type == INT_TYPE) {
            int_type_counter += 1;
        } else {
            float_type_counter += 1;
        }
    }

    int on_stack_int_type_counter = (int_type_counter - 6 >= 0) ? int_type_counter - 6 : 0;
    int on_stack_float_type_counter = (float_type_counter - 8 >= 0) ? float_type_counter - 8 : 0;
    int on_stack_alignment = (on_stack_int_type_counter + on_stack_float_type_counter) % 2;

    size_t total_size = 2 * 8 + 1 + 1 + 1 + 6 + on_stack_alignment + on_stack_int_type_counter
            + on_stack_float_type_counter;
    unsigned long long* result_row = new unsigned long long[total_size];
    unsigned long long* result = result_row + 2 * 8 + 1;
    
    result[-1] = float_type_counter;
    result[0] = on_stack_int_type_counter + on_stack_float_type_counter + on_stack_alignment;
    result[1] = func_address;

    int current_int_type_counter = 0;
    int current_float_type_counter = 0;
    int current_stack_position = total_size - 2 * 8 - 1 - 1;
    for (int i = 0; i < len; i++) {
        if (args_with_types[i].type == INT_TYPE) {
            if (current_int_type_counter < 6) {
                result[2 + current_int_type_counter] = args_with_types[i].value;
            } else {
                result[current_stack_position] = args_with_types[i].value;
                current_stack_position -= 1;
            }
            current_int_type_counter += 1;
        } else {
            if (current_float_type_counter < 8) {
                result_row[2 * current_float_type_counter] = args_with_types[i].value;
            } else {
                result[current_stack_position] = args_with_types[i].value;
                current_stack_position -= 1;
            }
            current_float_type_counter += 1;
        }
    }
    
    return result;
}

void
call(const uintptr_t func_address, const vector<ValueAndType>& args)
{
    unsigned long long* raw_args = argswt2args(func_address, args);
    assembly_call(raw_args);
    delete[] (raw_args - 2 * 8 - 1);
}

bool
is_string(const char* s, const size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if ((s[i] != '.') && (((isdigit(s[i]) == false) && (i != 0)) || ((i == 0) && (s[i] != '-')
                && (s[i] != '+') && (isdigit(s[i]) == false)))) {
            return true;
        }
    }
    return false;
}

void
readValue(ValueAndType& vat, const char* s)
{
    size_t n = strlen(s);
    if (is_string(s, n)) {
        vat.buffer_for_string = new char[n];
        vat.value = (unsigned long long)vat.buffer_for_string;
        memcpy(vat.buffer_for_string, s, n);
    } else if (strchr(s, '.') == nullptr) {
        stringstream ss(s);
        ss >> vat.value;
    } else {
        vat.type = FLOAT_TYPE;
        stringstream ss(s);
        double value;
        ss >> value;
        memcpy(&vat.value, &value, sizeof(value));
    }
}

uintptr_t
getfunc_address(const char* lib_name, const char* func_name)
{
    void* result;
    void* handle = dlopen(lib_name, RTLD_LAZY); 
    if (handle == nullptr) {
        // cerr << dlerror() << endl;
        cerr << "Wrong library location: " << lib_name << endl;
        return 0;
    }
    result = dlsym(handle, func_name);
    if (dlerror() != nullptr) {
        // cerr << dlerror() << endl;
        cerr << "There is no function " << func_name << " in " << lib_name << endl;
        return 0;
    }
    dlclose(handle);
    return uintptr_t(result);
}

int
main(int argc, char** argv)
{
    const char* lib_name = "/lib/x86_64-linux-gnu/libc.so.6";
    if (argc <= 1) {
        cerr << "Not enough arguments!" << endl;
        return 0;
    }
    int current_arg_idx = 1;
    if ((argv[current_arg_idx][0] == '.') || (argv[current_arg_idx][0] == '/')
            || (argv[current_arg_idx][0] == '~')) {
        lib_name = argv[current_arg_idx];
        current_arg_idx++;
    }
    const char* func_name = argv[current_arg_idx];
    current_arg_idx++;

    uintptr_t func_address = getfunc_address(lib_name, func_name);
    if (func_address == 0) {
        return 0;
    }

    vector<ValueAndType> args;
    for ( ; current_arg_idx < argc; current_arg_idx++) {
        ValueAndType arg;
        readValue(arg, argv[current_arg_idx]);
        args.push_back(arg);
    }    
    
    call(func_address, args);

    for (auto& elem : args) {
        if (elem.buffer_for_string != nullptr) {
            delete[] elem.buffer_for_string;
        }
    }
    return 0;
}

#!/usr/bin/python
# -*- coding: utf-8 -*-

"""Functions used to generate source files during build time

All such functions are invoked in a subprocess on Windows to prevent build flakiness.

"""
from platform_methods import subprocess_main


class RDHeaderStruct:
    def __init__(self):
        self.vertex_lines = []
        self.fragment_lines = []
        self.tesc_lines = []
        self.tese_lines = []
        self.compute_lines = []
        self.task_lines = []
        self.mesh_lines = []

        self.vertex_included_files = []
        self.fragment_included_files = []
        self.compute_included_files = []
        self.tesc_included_files = []
        self.tese_included_files = []
        self.task_included_files = []
        self.mesh_included_files = []

        self.reading = ""
        self.line_offset = 0
        self.vertex_offset = 0
        self.fragment_offset = 0
        self.tesc_offset = 0
        self.tese_offset = 0
        self.task_offset = 0
        self.mesh_offset = 0
        self.compute_offset = 0


def include_file_in_rd_header(filename, header_data, depth):
    fs = open(filename, "r")
    line = fs.readline()

    while line:

        index = line.find("//")
        if index != -1:
            line = line[:index]

        if line.find("#[vertex]") != -1:
            header_data.reading = "vertex"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.vertex_offset = header_data.line_offset
            continue

        if line.find("#[fragment]") != -1:
            header_data.reading = "fragment"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.fragment_offset = header_data.line_offset
            continue

        if line.find("#[compute]") != -1:
            header_data.reading = "compute"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.compute_offset = header_data.line_offset
            continue

        if line.find("#[tesc]") != -1:
            header_data.reading = "tesc"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.tesc_offset = header_data.line_offset
            continue
        if line.find("#[tese]") != -1:
            header_data.reading = "tese"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.tese_offset = header_data.line_offset
            continue
        if line.find("#[task]") != -1:
            header_data.reading = "task"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.task_offset = header_data.line_offset
            continue
        if line.find("#[mesh]") != -1:
            header_data.reading = "mesh"
            line = fs.readline()
            header_data.line_offset += 1
            header_data.mesh_offset = header_data.line_offset
            continue

        while line.find("#include ") != -1:
            includeline = line.replace("#include ", "").strip()[1:-1]

            import os.path

            included_file = ""

            if includeline.startswith("thirdparty/"):
                included_file = os.path.relpath(includeline)

            else:
                included_file = os.path.relpath(
                    os.path.dirname(filename) + "/" + includeline)

            if not included_file in header_data.vertex_included_files and header_data.reading == "vertex":
                header_data.vertex_included_files += [included_file]
                if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.fragment_included_files and header_data.reading == "fragment":
                header_data.fragment_included_files += [included_file]
                if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.compute_included_files and header_data.reading == "compute":
                header_data.compute_included_files += [included_file]
                if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.tesc_included_files and header_data.reading == "tesc":
                header_data.tesc_included_files += [included_file]
                if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.tese_included_files and header_data.reading == "tese":
                header_data.tese_included_files += [included_file]
                if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.task_included_files and header_data.reading == "task":
                header_data.task_included_files += [included_file]
                if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")
            elif not included_file in header_data.mesh_included_files and header_data.reading == "mesh":
                header_data.mesh_included_files += [included_file]
                if include_file_in_rd_header(included_file, header_data, depth + 1) is None:
                    print("Error in file '" + filename + "': #include " +
                          includeline + "could not be found!")

            line = fs.readline()

        line = line.replace("\r", "")
        line = line.replace("\n", "")
        str_find_index = line.find("//")
        if(str_find_index >= 0):
            if(str_find_index > 0):
                line = line[0:str_find_index]
            else:
                line = ""

        if header_data.reading == "vertex":
            header_data.vertex_lines += [line]
        if header_data.reading == "fragment":
            header_data.fragment_lines += [line]
        if header_data.reading == "compute":
            header_data.compute_lines += [line]
        if header_data.reading == "tesc":
            header_data.tesc_lines += [line]
        if header_data.reading == "tese":
            header_data.tese_lines += [line]
        if header_data.reading == "task":
            header_data.task_lines += [line]
        if header_data.reading == "mesh":
            header_data.mesh_lines += [line]

        line = fs.readline()
        header_data.line_offset += 1

    fs.close()

    return header_data


def build_rd_header(filename):
    print("build_rd_header:" + filename)
    header_data = RDHeaderStruct()
    include_file_in_rd_header(filename, header_data, 0)

    out_file = filename + ".gen.h"
    fd = open(out_file, "w")

    fd.write("/* WARNING, THIS FILE WAS GENERATED, DO NOT EDIT */\n")

    out_file_base = out_file
    out_file_base = out_file_base[out_file_base.rfind("/") + 1:]
    out_file_base = out_file_base[out_file_base.rfind("\\") + 1:]
    out_file_ifdef = out_file_base.replace(".", "_").upper()
    fd.write("#ifndef " + out_file_ifdef + "_RD\n")
    fd.write("#define " + out_file_ifdef + "_RD\n")

    out_file_class = out_file_base.replace(".glsl.gen.h", "").title().replace(
        "_", "").replace(".", "") + "ShaderRD"
    fd.write("\n")
    fd.write('#include "servers/rendering/renderer_rd/shader_rd.h"\n\n')
    fd.write("class " + out_file_class + " : public ShaderRD {\n\n")
    fd.write("public:\n\n")

    fd.write("\t" + out_file_class + "() {\n\n")

    if len(header_data.compute_lines):

        curr_line = 0
        curr_index = 0
        fd.write("\t\tstatic const char _compute_code[] = {\n")
        for x in header_data.compute_lines:
            curr_line += 1
            curr_index = 0
            for c in x:
                curr_index += 1
                code = ord(c)
                # 检查是否存在错误编码
                if code > 255:
                    print("shader error : compute shader :" + filename + ":(" +
                          str(curr_line) + ") index:" + str(curr_index) + " code error， code :" + str(code) + "  char :" + c + "line:" + x)
                fd.write(str(code) + ",")
            fd.write(str(ord("\n")) + ",")
        fd.write("\t\t0};\n\n")

        fd.write('\t\tsetup(nullptr, nullptr, _compute_code, "' +
                 out_file_class + '");\n')
        fd.write("\t}\n")

    else:

        fd.write("\t\tstatic const char _vertex_code[] = {\n")
        curr_line = 0
        curr_index = 0
        for x in header_data.vertex_lines:
            curr_line += 1
            curr_index = 0
            for c in x:
                curr_index += 1
                code = ord(c)
                # 检查是否存在错误编码
                if code > 255:
                    print("shader error : vertex shader :" + filename + ":(" +
                          str(curr_line) + ") index:" + str(curr_index) + " code error， code :" + str(code) + "  char :" + c + "line:" + x)
                fd.write(str(ord(c)) + ",")
            fd.write(str(ord("\n")) + ",")
        fd.write("\t\t0};\n\n")

        fd.write("\t\tstatic const char _fragment_code[]={\n")
        for x in header_data.fragment_lines:
            for c in x:
                fd.write(str(ord(c)) + ",")
            fd.write(str(ord("\n")) + ",")
        fd.write("\t\t0};\n\n")
        # 解析曲面细分
        use_tesc = 0
        use_tese = 0
        use_task = 0
        use_mesh = 0
        if len(header_data.tesc_lines):
            curr_line = 0
            curr_index = 0
            use_tesc = 1
            fd.write("\t\tstatic const char _tesc_code[] = {\n")
            for x in header_data.tesc_lines:
                curr_line += 1
                curr_index = 0
                for c in x:
                    curr_index += 1
                    code = ord(c)
                    # 检查是否存在错误编码
                    if code > 255:
                        print("shader error : tesc shader :" + filename + ":(" +
                              str(curr_line) + ") index:" + str(curr_index) + " code error， code :" + str(code) + "  char :" + c + "line:" + x)
                    fd.write(str(code) + ",")
                fd.write(str(ord("\n")) + ",")
            fd.write("\t\t0};\n\n")

        if len(header_data.tese_lines):
            fd.write("\t\tstatic const char _tese_code[] = {\n")
            curr_line = 0
            curr_index = 0
            use_tese = 1
            for x in header_data.tese_lines:
                curr_line += 1
                curr_index = 0
                for c in x:
                    curr_index += 1
                    code = ord(c)
                    # 检查是否存在错误编码
                    if code > 255:
                        print("shader error : tese shader :" + filename + ":(" +
                              str(curr_line) + ") index:" + str(curr_index) + " code error， code :" + str(code) + "  char :" + c + "line:" + x)

                    fd.write(str(code) + ",")
                fd.write(str(ord("\n")) + ",")
            fd.write("\t\t0};\n\n")
        if len(header_data.task_lines):
            fd.write("\tstatic const char _task_code[] = {\n")
            curr_line = 0
            curr_index = 0
            use_task = 1
            for x in header_data.task_lines:
                for c in x:
                    curr_index += 1
                    code = ord(c)
                    # 检查是否存在错误编码
                    if code > 255:
                        print("shader error : task shader :" + filename + ":(" +
                              str(curr_line) + ") index:" + str(curr_index) + " code error， code :" + str(code) + "  char :" + c + "line:" + x)
                    fd.write(str(ord(c)) + ",")
                fd.write(str(ord("\n")) + ",")
            fd.write("\t\t0};\n\n")
            fd.write("\t\tsetup_task(_task_code);\n")
        if len(header_data.mesh_lines):
            fd.write("\tstatic const char _mesh_code[] = {\n")
            curr_line = 0
            curr_index = 0
            use_mesh = 1
            for x in header_data.mesh_lines:
                for c in x:
                    curr_index += 1
                    code = ord(c)
                    # 检查是否存在错误编码
                    if code > 255:
                        print("shader error : mesh shader :" + filename + ":(" +
                              str(curr_line) + ") index:" + str(curr_index) + " code error， code :" + str(code) + "  char :" + c + "line:" + x)
                    fd.write(str(ord(c)) + ",")
                fd.write(str(ord("\n")) + ",")
            fd.write("\t\t0};\n\n")
            fd.write("\t\tsetup_mesh(_mesh_code);\n")
        if use_tesc and use_tese:
            fd.write(
                '\t\tsetup(_vertex_code, _fragment_code, nullptr, "' + out_file_class + '", _tesc_code, _tese_code);\n')
        elif use_task or use_mesh:
            task_name = "nullptr"
            mesh_name = "nullptr"
            if use_task:
                task_name = "_task_code"
            if use_mesh:
                mesh_name = "_mesh_code"
            fd.write(
                '\t\tsetup(_vertex_code, _fragment_code, nullptr, "' + out_file_class + ", nullptr, nullptr," + task_name + "," + mesh_name + ");\n")
        
        else:
            fd.write(
                '\t\tsetup(_vertex_code, _fragment_code, nullptr, "' + out_file_class + '");\n')
        fd.write("\t}\n")


    fd.write("};\n\n")

    fd.write("#endif\n")
    fd.close()


def build_rd_headers(target, source, env):
    for x in source:
        build_rd_header(str(x))


class RAWHeaderStruct:
    def __init__(self):
        self.code = ""


def include_file_in_raw_header(filename, header_data, depth):
    fs = open(filename, "r")
    line = fs.readline()

    while line:

        while line.find("#include ") != -1:
            includeline = line.replace("#include ", "").strip()[1:-1]

            import os.path

            included_file = os.path.relpath(
                os.path.dirname(filename) + "/" + includeline)
            include_file_in_raw_header(included_file, header_data, depth + 1)

            line = fs.readline()

        header_data.code += line
        line = fs.readline()

    fs.close()


def build_raw_header(filename):
    print("build_raw_header:" + filename)
    header_data = RAWHeaderStruct()
    include_file_in_raw_header(filename, header_data, 0)

    out_file = filename + ".gen.h"
    fd = open(out_file, "w")

    fd.write("/* WARNING, THIS FILE WAS GENERATED, DO NOT EDIT */\n")

    out_file_base = out_file.replace(".glsl.gen.h", "_shader_glsl")
    out_file_base = out_file_base[out_file_base.rfind("/") + 1:]
    out_file_base = out_file_base[out_file_base.rfind("\\") + 1:]
    out_file_ifdef = out_file_base.replace(".", "_").upper()
    fd.write("#ifndef " + out_file_ifdef + "_RAW_H\n")
    fd.write("#define " + out_file_ifdef + "_RAW_H\n")
    fd.write("\n")
    fd.write("static const char " + out_file_base + "[] = {\n")
    for c in header_data.code:
        fd.write(str(ord(c)) + ",")
    fd.write("\t\t0};\n\n")
    fd.write("#endif\n")
    fd.close()


def build_raw_headers(target, source, env):
    for x in source:
        build_raw_header(str(x))


if __name__ == "__main__":
    subprocess_main(globals())

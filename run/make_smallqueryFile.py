#!/usr/bin/env python3
import random

OUT_FILE = "queryFileT9_small_multi"

# Enough to force multiple pages, but not huge
N_STUDENT = 120    # ~10–12 pages for 40-byte records
N_COURSE  = 40
N_PROF    = 60
N_ENROLL  = 200

NUM_OPS = ["=", "<>", "<", "<=", ">", ">="]
STR_OPS = ["=", "<>"]


def random_dept():
    return random.choice(["CS", "EE", "ME"])


def random_semester():
    return random.choice(["Fall2024", "Spring2025", "Fall2025", "Spring2026"])


def random_gpa():
    return round(random.uniform(2.0, 4.0), 2)


def random_num_op():
    return random.choice(NUM_OPS)


def random_str_op():
    return random.choice(STR_OPS)


def write_header(f):
    f.write("createdb DBNAME;\n")
    f.write("opendb DBNAME;\n\n")

    f.write("create Student(ID=i, Name=s16, Dept=s16, GPA=f);\n")
    f.write("create Professor(Faculty=s16, Dept=s16, SID=i);\n")
    f.write("create Course(Course=s16, Credits=i, Dept=s16);\n")
    f.write("create Enrollment(SID=i, Course=s16, Semester=s16);\n\n")


def write_footer(f):
    f.write("print Student;\n")
    f.write("print Professor;\n")
    f.write("print Course;\n")
    f.write("print Enrollment;\n")
    f.write("closedb;\n")
    f.write("destroydb DBNAME;\n")
    f.write("quit;\n")


def generate_students(f):
    f.write("-- Students: multi-page inserts + bursts of deletes\n")
    for sid in range(1, N_STUDENT + 1):
        name = f"S{sid}"           # short, fits s16
        dept = random_dept()
        gpa  = random_gpa()

        f.write(
            f'insert into Student(ID={sid}, Name="{name}", Dept="{dept}", GPA={gpa:.2f});\n'
        )

        # Every 10 inserts, emit 2–3 deletes in a row
        if sid >= 20 and sid % 10 == 0:
            num_deletes = random.randint(2, 3)
            for _ in range(num_deletes):
                attr = random.choice(["ID", "GPA", "Dept"])
                if attr == "ID":
                    op = random_num_op()
                    val = random.randint(1, N_STUDENT + 20)
                    f.write(f"delete from Student where ({attr}{op}{val});\n")
                elif attr == "GPA":
                    op = random_num_op()
                    val = random_gpa()
                    f.write(f"delete from Student where ({attr}{op}{val:.2f});\n")
                else:  # Dept
                    op = random_str_op()
                    val = random_dept()
                    f.write(f'delete from Student where ({attr}{op}"{val}");\n')

    f.write("\n")


def generate_courses(f):
    f.write("-- Courses: multi-page-ish inserts + deletes\n")
    for cid in range(1, N_COURSE + 1):
        course_name = f"C{cid:03d}"  # C001, C002...
        dept = random_dept()
        credits = random.choice([3, 4])

        f.write(
            f'insert into Course(Course="{course_name}", Credits={credits}, Dept="{dept}");\n'
        )

        # Every 8 inserts, 1–2 deletes
        if cid >= 16 and cid % 8 == 0:
            num_deletes = random.randint(1, 2)
            for _ in range(num_deletes):
                attr = random.choice(["Course", "Credits", "Dept"])
                if attr == "Credits":
                    op = random_num_op()
                    val = random.choice([2, 3, 4, 5])
                    f.write(f"delete from Course where ({attr}{op}{val});\n")
                elif attr == "Course":
                    op = random_str_op()
                    del_id = random.randint(1, N_COURSE + 10)
                    val = f"C{del_id:03d}"
                    f.write(f'delete from Course where ({attr}{op}"{val}");\n')
                else:  # Dept
                    op = random_str_op()
                    val = random_dept()
                    f.write(f'delete from Course where ({attr}{op}"{val}");\n')

    f.write("\n")


def generate_professors(f):
    f.write("-- Professors: enough rows + deletes\n")
    for pid in range(1, N_PROF + 1):
        name = f"Prof{pid}"
        dept = random_dept()
        sid  = random.randint(1, N_STUDENT)

        f.write(
            f'insert into Professor(Faculty="{name}", Dept="{dept}", SID={sid});\n'
        )

        # Every 12 inserts, 2–3 deletes
        if pid >= 24 and pid % 12 == 0:
            num_deletes = random.randint(2, 3)
            for _ in range(num_deletes):
                attr = random.choice(["Faculty", "Dept", "SID"])
                if attr == "SID":
                    op = random_num_op()
                    val = random.randint(1, N_STUDENT + 50)
                    f.write(f"delete from Professor where ({attr}{op}{val});\n")
                elif attr == "Faculty":
                    op = random_str_op()
                    del_id = random.randint(1, N_PROF + 20)
                    val = f"Prof{del_id}"
                    f.write(f'delete from Professor where ({attr}{op}"{val}");\n')
                else:  # Dept
                    op = random_str_op()
                    val = random_dept()
                    f.write(f'delete from Professor where ({attr}{op}"{val}");\n')

    f.write("\n")


def generate_enrollments(f):
    f.write("-- Enrollments: many rows across pages + deletes\n")
    for eid in range(1, N_ENROLL + 1):
        sid = random.randint(1, N_STUDENT)
        cid = random.randint(1, N_COURSE)
        course_name = f"C{cid:03d}"
        sem = random_semester()

        f.write(
            f'insert into Enrollment(SID={sid}, Course="{course_name}", Semester="{sem}");\n'
        )

        # Every 15 inserts, 2–4 deletes in a row
        if eid >= 30 and eid % 15 == 0:
            num_deletes = random.randint(2, 4)
            for _ in range(num_deletes):
                attr = random.choice(["SID", "Course", "Semester"])
                if attr == "SID":
                    op = random_num_op()
                    val = random.randint(1, N_STUDENT + 50)
                    f.write(f"delete from Enrollment where ({attr}{op}{val});\n")
                elif attr == "Course":
                    op = random_str_op()
                    del_cid = random.randint(1, N_COURSE + 20)
                    val = f"C{del_cid:03d}"
                    f.write(f'delete from Enrollment where ({attr}{op}"{val}");\n')
                else:  # Semester
                    op = random_str_op()
                    val = random_semester()
                    f.write(f'delete from Enrollment where ({attr}{op}"{val}");\n')

    f.write("\n")


def main():
    random.seed(12345)  # deterministic
    with open(OUT_FILE, "w") as f:
        write_header(f)
        generate_students(f)
        generate_courses(f)
        generate_professors(f)
        generate_enrollments(f)
        write_footer(f)


if __name__ == "__main__":
    main()
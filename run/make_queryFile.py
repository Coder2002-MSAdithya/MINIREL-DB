#!/usr/bin/env python3
import random

# Rough page estimate:
# Student record: ID=i(4) + Name=s16(16) + Dept=s16(16) + GPA=f(4) = 40 bytes
# Others around ~36 bytes.
# With 512-byte pages and header, ~10–12 records/page.
# We overshoot to ensure ~1000 pages per table.

PAGE_TARGET = 1000              # target pages per table
EST_RECS_PER_PAGE = 12          # rough records per page

N_STUDENT = PAGE_TARGET * EST_RECS_PER_PAGE + 20000
N_PROF    = PAGE_TARGET * EST_RECS_PER_PAGE
N_COURSE  = PAGE_TARGET * EST_RECS_PER_PAGE
N_ENROLL  = PAGE_TARGET * EST_RECS_PER_PAGE * 2

OUT_FILE = "queryFileT9"

# NOTE: using <> for inequality, as per your catalog
NUM_OPS = ["=", "<>", "<", "<=", ">", ">="]
STR_OPS = ["=", "<>"]


def write_header(f):
    f.write("createdb DBNAME;\n")
    f.write("opendb DBNAME;\n")
    f.write("\n")

    # Schemas as per your earlier example (fixed-size records)
    f.write("create Student(ID=i, Name=s16, Dept=s16, GPA=f);\n")
    f.write("create Professor(Faculty=s16, Dept=s16, SID=i);\n")
    f.write("create Course(Course=s16, Credits=i, Dept=s16);\n")
    f.write("create Enrollment(SID=i, Course=s16, Semester=s16);\n")
    f.write("\n")


def write_footer(f):
    # Some prints at the end, plus close/destroy/quit
    f.write("print Student;\n")
    f.write("print Professor;\n")
    f.write("print Course;\n")
    f.write("print Enrollment;\n")
    f.write("closedb;\n")
    f.write("destroydb DBNAME;\n")
    f.write("quit;\n")


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


def generate_students(f):
    f.write("; Massive inserts into Student with interspersed deletes\n")
    for sid in range(1, N_STUDENT + 1):
        name = f"S{sid}"          # safe for s16
        dept = random_dept()
        gpa  = random_gpa()

        f.write(
            f'insert into Student(ID={sid}, Name="{name}", Dept="{dept}", GPA={gpa:.2f});\n'
        )

        # Every now and then, emit 1–3 deletes in a row
        if sid > 100 and sid % 40 == 0:
            num_deletes = random.randint(1, 3)
            for _ in range(num_deletes):
                # Choose attribute to delete on
                attr = random.choice(["ID", "GPA", "Dept"])
                if attr == "ID":
                    op = random_num_op()
                    val = random.randint(1, sid + 1000)  # may or may not exist
                    f.write(
                        f"delete from Student where ( {attr} {op} {val} );\n"
                    )
                elif attr == "GPA":
                    op = random_num_op()
                    val = round(random.uniform(2.0, 4.0), 2)
                    f.write(
                        f"delete from Student where ( {attr} {op} {val:.2f} );\n"
                    )
                else:  # Dept
                    op = random_str_op()
                    val = random_dept()
                    f.write(
                        f'delete from Student where ( {attr} {op} "{val}" );\n'
                    )

    f.write("\n")


def generate_courses(f):
    f.write("; Massive inserts into Course with interspersed deletes\n")
    for cid in range(1, N_COURSE + 1):
        course_name = f"C{cid:05d}"   # C00001, etc.
        dept = random_dept()
        credits = random.choice([3, 4])

        f.write(
            f'insert into Course(Course="{course_name}", Credits={credits}, Dept="{dept}");\n'
        )

        if cid > 100 and cid % 45 == 0:
            num_deletes = random.randint(1, 3)
            for _ in range(num_deletes):
                attr = random.choice(["Course", "Credits", "Dept"])
                if attr == "Credits":
                    op = random_num_op()
                    val = random.choice([2, 3, 4, 5])
                    f.write(
                        f"delete from Course where ( {attr} {op} {val} );\n"
                    )
                elif attr == "Course":
                    op = random_str_op()
                    del_id = random.randint(1, cid + 1000)
                    val = f"C{del_id:05d}"
                    f.write(
                        f'delete from Course where ( {attr} {op} "{val}" );\n'
                    )
                else:  # Dept
                    op = random_str_op()
                    val = random_dept()
                    f.write(
                        f'delete from Course where ( {attr} {op} "{val}" );\n'
                    )

    f.write("\n")


def generate_professors(f):
    f.write("; Massive inserts into Professor with interspersed deletes\n")
    for pid in range(1, N_PROF + 1):
        name = f"Prof{pid}"
        dept = random_dept()
        sid  = random.randint(1, N_STUDENT)

        f.write(
            f'insert into Professor(Faculty="{name}", Dept="{dept}", SID={sid});\n'
        )

        if pid > 100 and pid % 50 == 0:
            num_deletes = random.randint(1, 3)
            for _ in range(num_deletes):
                attr = random.choice(["Faculty", "Dept", "SID"])
                if attr == "SID":
                    op = random_num_op()
                    val = random.randint(1, N_STUDENT + 500)
                    f.write(
                        f"delete from Professor where ( {attr} {op} {val} );\n"
                    )
                elif attr == "Faculty":
                    op = random_str_op()
                    del_id = random.randint(1, pid + 1000)
                    val = f"Prof{del_id}"
                    f.write(
                        f'delete from Professor where ( {attr} {op} "{val}" );\n'
                    )
                else:  # Dept
                    op = random_str_op()
                    val = random_dept()
                    f.write(
                        f'delete from Professor where ( {attr} {op} "{val}" );\n'
                    )

    f.write("\n")


def generate_enrollment(f):
    f.write("; Massive inserts into Enrollment with interspersed deletes\n")
    max_course_id = N_COURSE

    for eid in range(1, N_ENROLL + 1):
        sid = random.randint(1, N_STUDENT)
        cid = random.randint(1, max_course_id)
        course_name = f"C{cid:05d}"
        sem = random_semester()

        f.write(
            f'insert into Enrollment(SID={sid}, Course="{course_name}", Semester="{sem}");\n'
        )

        if eid > 100 and eid % 35 == 0:
            num_deletes = random.randint(1, 3)
            for _ in range(num_deletes):
                attr = random.choice(["SID", "Course", "Semester"])
                if attr == "SID":
                    op = random_num_op()
                    val = random.randint(1, N_STUDENT + 500)
                    f.write(
                        f"delete from Enrollment where ( {attr} {op} {val} );\n"
                    )
                elif attr == "Course":
                    op = random_str_op()
                    del_cid = random.randint(1, max_course_id + 1000)
                    val = f"C{del_cid:05d}"
                    f.write(
                        f'delete from Enrollment where ( {attr} {op} "{val}" );\n'
                    )
                else:  # Semester
                    op = random_str_op()
                    val = random_semester()
                    f.write(
                        f'delete from Enrollment where ( {attr} {op} "{val}" );\n'
                    )

    f.write("\n")


def main():
    random.seed(42)  # reproducible
    with open(OUT_FILE, "w") as f:
        write_header(f)
        generate_students(f)
        generate_courses(f)
        generate_professors(f)
        generate_enrollment(f)
        write_footer(f)


if __name__ == "__main__":
    main()
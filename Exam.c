#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN 32
#define AGE_MIN 5
#define AGE_MAX 75

// --- File where students are stored ---
static const char *EXAM_FILE = "Students.bin";

// --- Student data structure ---
typedef struct {
  uint32_t id;
  uint8_t age;
  char name[NAME_LEN + 1];
} student_t;

// --- Input and validation function declarations ---
static bool read_line(char *buffer, size_t size);
static void trim(char *text);
static bool parse_uint32(const char *text, uint32_t min, uint32_t max, uint32_t *value);
static bool s_age(uint8_t *age, const char *message, bool allow_empty, bool *provided);
static bool s_name(char *name, const char *message, bool allow_empty, bool *provided);
static bool s_id(uint32_t *id);

// --- ID, printing, and editing function declarations ---
static uint32_t next_student_id(void);
static void print_student(const student_t *student);
static void print_all_students(void);
static void create_student(void);
static void print_student_by_id(void);
static void edit_student(void);

// --- Main menu ---
int main(void) {
  printf("Student Records\n");
  while (true) {
    printf("\n");
    printf("A) Print All Students\n");
    printf("C) Create A Student\n");
    printf("P) Print A Student\n");
    printf("E) Edit A Student\n");
    printf("Q) Quit\n\n");
    printf("Select an option: \n");

    char input[32];
    if (!read_line(input, sizeof(input))) {
      printf("\nInput stream closed. Exiting.");
      break;
    }
    if (input[0] == '\0') {
      printf("Please choose an option.");
      continue;
    }

    const char choice = (char)toupper((unsigned char)input[0]);
    switch (choice) {
    case 'A':
      print_all_students();
      break;
    case 'C':
      create_student();
      break;
    case 'P':
      print_student_by_id();
      break;
    case 'E':
      edit_student();
      break;
    case 'Q':
      printf("Goodbye!\n");
      return 0;
    default:
      printf("Invalid option. Please try again.");
      break;
    }
  }
  return 0;
}

// --- Read line and remove newline ---
static bool read_line(char *buffer, size_t size) {
  if (fgets(buffer, (int)size, stdin) == NULL) {
    return false;
  }
  const size_t len = strcspn(buffer, "\n");
  if (buffer[len] == '\n') {
    buffer[len] = '\0';
  } else {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
  }
  return true;
}

// --- Remove extra spaces at start and end ---
static void trim(char *text) {
  if (text == NULL || text[0] == '\0') {
    return;
  }
  size_t len = strlen(text);
  while (len > 0 && isspace((unsigned char)text[len - 1])) {
    text[--len] = '\0';
  }
  size_t start = 0;
  while (text[start] != '\0' && isspace((unsigned char)text[start])) {
    start++;
  }
  if (start > 0) {
    memmove(text, text + start, len - start + 1);
  }
}

// --- Convert string to uint32 and check range ---
static bool parse_uint32(const char *text, uint32_t min, uint32_t max, uint32_t *value) {
  if (text == NULL || text[0] == '\0') {
    return false;
  }
  char *end = NULL;
  errno = 0;
  const unsigned long parsed = strtoul(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0') {
    return false;
  }
  if (parsed < min || parsed > max) {
    return false;
  }
  *value = (uint32_t)parsed;
  return true;
}

// --- Ask for age ---
static bool s_age(uint8_t *age, const char *message, bool allow_empty, bool *provided) {
  char buffer[32];
  while (true) {
    printf("%s", message);
    if (!read_line(buffer, sizeof(buffer))) {
      printf("Input aborted.");
      return false;
    }
    if (buffer[0] == '\0') {
      if (allow_empty) {
        if (provided != NULL) {
          *provided = false;
        }
        return true;
      }
      printf("Age cannot be empty.\n\n");
      continue;
    }
    uint32_t value = 0;
    if (!parse_uint32(buffer, AGE_MIN, AGE_MAX, &value)) {
      printf("Please enter a number between %d and %d.\n", AGE_MIN, AGE_MAX);
      continue;
    }
    *age = (uint8_t)value;
    if (provided != NULL) {
      *provided = true;
    }
    return true;
  }
}

// --- Ask for name ---
static bool s_name(char *name, const char *message, bool allow_empty, bool *provided) {
  char buffer[128];
  while (true) {
    printf("%s", message);
    if (!read_line(buffer, sizeof(buffer))) {
      printf("Input aborted.");
      return false;
    }
    trim(buffer);
    if (buffer[0] == '\0') {
      if (allow_empty) {
        if (provided != NULL) {
          *provided = false;
        }
        return true;
      }
      printf("Name cannot be empty.\n\n");
      continue;
    }
    const size_t len = strlen(buffer);
    if (len > NAME_LEN) {
      printf("Name must be at most %d characters.\n", NAME_LEN);
      continue;
    }
    strncpy(name, buffer, NAME_LEN);
    name[len] = '\0';
    if (provided != NULL) {
      *provided = true;
    }
    return true;
  }
}

// --- Ask for ID ---
static bool s_id(uint32_t *id) {
  char buffer[32];
  while (true) {
    printf("Enter ID: ");
    if (!read_line(buffer, sizeof(buffer))) {
      printf("Input aborted.");
      return false;
    }
    if (buffer[0] == '\0') {
      printf("ID cannot be empty.\n\n");
      continue;
    }
    if (!parse_uint32(buffer, 1, UINT32_MAX, id)) {
      printf("Please enter a positive integer.");
      continue;
    }
    return true;
  }
}

// --- Get next free ID by scanning file ---
static uint32_t next_student_id(void) {
  FILE *fp = fopen(EXAM_FILE, "rb");
  if (fp == NULL) {
    if (errno == ENOENT) {
      return 1;
    }
    perror("Failed to open student file");
    return 0;
  }
  uint32_t max_id = 0;
  student_t student;
  while (fread(&student, sizeof(student), 1, fp) == 1) {
    if (student.id > max_id) {
      max_id = student.id;
    }
  }
  if (ferror(fp)) {
    perror("Failed to read student file");
    max_id = 0;
  }
  fclose(fp);
  if (max_id == UINT32_MAX) {
    fputs("ID limit reached.\n", stderr);
    return 0;
  }
  return max_id + 1;
}

// --- Print one student ---
static void print_student(const student_t *student) {
  printf("%-5u | %-32s | %3u\n", student->id, student->name, student->age);
}

// --- Print all students from file ---
static void print_all_students(void) {
  FILE *fp = fopen(EXAM_FILE, "rb");
  if (fp == NULL) {
    if (errno == ENOENT) {
      printf("No students stored yet.\n");
    } else {
      perror("Failed to open student file");
    }
    return;
  }
  size_t count = 0;
  student_t student;
  while (fread(&student, sizeof(student), 1, fp) == 1) {
    if (count == 0) {
      puts("\nID    | Name                             | Age");
      puts("-------------------------------------------------");
    }
    print_student(&student);
    count++;
  }
  if (count == 0) {
    printf("No students stored yet.");
  } else if (ferror(fp)) {
    perror("Failed to read student file");
  }
  fclose(fp);
}

// --- Create a new student ---
static void create_student(void) {
  uint8_t age = 0;
  char name[NAME_LEN + 1];

  if (!s_age(&age, "Enter age (5-75): ", false, NULL)) {
    return;
  }
  if (!s_name(name, "Enter name (max 32 chars): ", false, NULL)) {
    return;
  }

  const uint32_t new_id = next_student_id();
  if (new_id == 0) {
    return;
  }

  student_t student = {.id = new_id, .age = age};
  strncpy(student.name, name, sizeof(student.name) - 1);
  student.name[sizeof(student.name) - 1] = '\0';

  FILE *fp = fopen(EXAM_FILE, "ab");
  if (fp == NULL) {
    perror("Failed to open student file");
    return;
  }
  if (fwrite(&student, sizeof(student), 1, fp) != 1) {
    perror("Failed to write student");
    fclose(fp);
    return;
  }
  fclose(fp);
  printf("Student created with ID %u.\n", student.id);
}

// --- Print a student by ID ---
static void print_student_by_id(void) {
  uint32_t student_id = 0;
  if (!s_id(&student_id)) {
    return;
  }

  FILE *fp = fopen(EXAM_FILE, "rb");
  if (fp == NULL) {
    if (errno == ENOENT) {
      printf("No students stored yet.");
    } else {
      perror("Failed to open student file");
    }
    return;
  }

  bool found = false;
  student_t student;
  while (fread(&student, sizeof(student), 1, fp) == 1) {
    if (student.id == student_id) {
      found = true;
      break;
    }
  }

  if (found) {
    printf("\nID    | Name                             | Age");
    printf("-------------------------------------------------");
    print_student(&student);
  } else if (ferror(fp)) {
    perror("Failed to read student file");
  } else {
    printf("No student found with ID %u.\n", student_id);
  }

  fclose(fp);
}

// --- Edit an existing student ---
static void edit_student(void) {
  uint32_t student_id = 0;
  if (!s_id(&student_id)) {
    return;
  }

  FILE *fp = fopen(EXAM_FILE, "rb+");
  if (fp == NULL) {
    if (errno == ENOENT) {
      printf("No students stored yet.");
    } else {
      perror("Failed to open student file");
    }
    return;
  }

  bool found = false;
  student_t student;
  while (fread(&student, sizeof(student), 1, fp) == 1) {
    if (student.id == student_id) {
      found = true;
      break;
    }
  }

  if (!found) {
    if (ferror(fp)) {
      perror("Failed to read student file");
    } else {
      printf("No student found with ID %u.\n", student_id);
    }
    fclose(fp);
    return;
  }

  printf("Editing student %u (%s, %u years old)\n", student.id, student.name, student.age);

  uint8_t new_age = student.age;
  bool age_updated = false;
  if (!s_age(&new_age, "Enter new age (5-75) or press ENTER to keep current: ", true, &age_updated)) {
    fclose(fp);
    return;
  }

  char new_name[NAME_LEN + 1];
  bool name_updated = false;
  if (!s_name(new_name, "Enter new name (max 32 chars) or press ENTER to keep current: ", true, &name_updated)) {
    fclose(fp);
    return;
  }

  if (!age_updated && !name_updated) {
    printf("No changes entered.");
    fclose(fp);
    return;
  }

  if (age_updated) {
    student.age = new_age;
  }
  if (name_updated) {
    strncpy(student.name, new_name, sizeof(student.name) - 1);
    student.name[sizeof(student.name) - 1] = '\0';
  }

  if (fseek(fp, -(long)sizeof(student), SEEK_CUR) != 0) {
    perror("Failed to seek student file");
    fclose(fp);
    return;
  }

  if (fwrite(&student, sizeof(student), 1, fp) != 1) {
    perror("Failed to update student");
    fclose(fp);
    return;
  }

  fclose(fp);
  printf("Student updated.");
}

README.txt - HW1: Pointers and Embedded Thinking

Name: Dhruv Naik 
Date: 1/19/2026

---

General Instructions:

- This file is required as part of your submission.
- Fill in the answers to the questions for each part.
- Use this document to also describe your code and paste output.
- Be clear and concise.

---

Part I: C Basics – Variables and Pointers

1. What is the difference between a pointer and a variable?  
Answer: A variable stores a data value, whereas a pointer is a type of variable that stores the memory address of another variable. 

2. What happens when two pointers point to the same variable?  
Answer: In this instance, both pointers will store the same memory address and either can be used to access the value of the variable. 

3. What is the benefit of accessing values through pointers?  
Answer: Pointers allow for Dynamic Memory Allocation to manage a data structure like a list more efficiency or dynamic resizing of arrays. 

4. Copy your Serial Monitor output from Part I below:  
(Paste here)

a = 100, *p = 100
Address of a: 0x3fcebcec
a = 77, *p = 77, *q = 77
Original b = 200
New b = 999, and a = 77

---

Part II: Memory and Arrays

1. How are elements in an array laid out in memory? How do they differ between types (e.g., char vs int)?  
Answer: All elements in an array are stored in a contiguous block of memory. The difference between a char and int array is that for a char array, each element is 1 byte while for a int array each element is 4 bytes, therefore the arrays are addressed as such.

2. How does your my_memcpy() function work under the hood?  
Answer: Under the hood the mymemcpy() function casts the void pointers to byte size so that in the for loop each byte of memory is copied one by one into the new memory location.   

3. What situations in embedded systems might require memory-level copying?  
Answer: Since embedded systems might require using hardware with less resources than standard computers, memory-level copying can help reduce overhead. It also increases speed which could be necessary in systems that require functions to run as fast as possible such as stock trading. 

4. Copy your Serial Monitor output from Part II below:  
(Paste here)

Integer array:
arr[0] = 10, address: 0x3fcebcb8
arr[1] = 20, address: 0x3fcebcbc
arr[2] = 30, address: 0x3fcebcc0
arr[3] = 40, address: 0x3fcebcc4
arr[4] = 50, address: 0x3fcebcc8

Char array:
text[0] = a, address: 0x3fcebcb3
text[1] = b, address: 0x3fcebcb4
text[2] = c, address: 0x3fcebcb5
text[3] = d, address: 0x3fcebcb6
text[4] = e, address: 0x3fcebcb7

Copied string: hello world

---

Part III: Structs and Simulated Sensor Records

1. What is a pointer to a struct, and how is it used?  
Answer: A pointer to a struct is a variable that stores the memory address of a structure. It is used to access and change members of the struct. The pointer is useful when passing structs to a function to avoid copying.

2. How does dynamic memory allocation work in C?  
Answer: Dynamic memory allocation in C lets a program allocate, resize and free memory at runtime. This allocates memory in the heap instead of the stack by requesting from the operating system. 

3. Why is dynamic allocation useful in embedded systems?  
Answer: Dynamic allocation is useful when the amount of memory required for a task is unknown at compile time. Since embedded systems typically have little memory dynamic allocation can reduce memory usage by allowing the system to adapt during runtime. 

4. Copy your Serial Monitor output from Part III below:  
(Paste here)

Sensor ID: 1, Label: TEMP, Reading: 25.60
Updated reading via pointer: 42.90
Sensor ID: 1, Label: TEMP, Reading: 42.90

Allocating 3 sensors...
Sensor 0 - ID: 100, Label: T0, Reading: 20.00
Sensor 1 - ID: 101, Label: T1, Reading: 21.50
Sensor 2 - ID: 102, Label: T2, Reading: 23.00

Anything else you'd like us to know? (bugs, challenges, feedback, etc.):  
(Optional)
Nope :)
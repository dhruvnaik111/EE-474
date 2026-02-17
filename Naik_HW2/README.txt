README - Homework 2 Answers
Dhruv Naik

Part I:
- Why use pointer arithmetic instead of array indexing in some cases?
    If you are passing a pointer as a function argument it can be usefull to use it to traverse an array without having to
    create additional variables to track an index. 
- How do you return multiple values from a function in C?
    Since C functions can only return a single value you can either use pointers or structs. 
    With pointers you can pass the addresses to the function and then change the values by dereferencing. Otherwise you can use a struct where
    you can return a struct object.
- What challenges arise when skipping elements with pointer arithmetic?
    Skipping elements with pointer arithmetic risks going out of bounds of the array leading to undefined behavior. 

Part II:
- How is a 2D array laid out in memory?
    In C 2D arrays are stored in contigous memory in row-major order. For example the elements would be stored as (row)(col): [0][0] [0][1] [1][0] [1][1].
- Why might you flatten a 2D array in embedded code?
    Flattening a 2D array in embeded code as typically embedded systems rely on infrastructure with little available memory.
    In a flattened array one can direct access the memory locations which is faster than array indexing. 
- What’s the difference between matrix[i][j] and *(&matrix[0][0] + i * cols + j)?
    matrix[i][j] uses array indexing while *(&matrix[0][0] + i * cols + j) uses pointer arithmetic to get values from the array. Both will perform the same task in a contigous array. 

Part III:
- What’s the purpose of the head and tail indices?
    The head points to where the next element will be pushed to, 
    while the tail points to the position of the element that would be popped.
    This allows the buffer to push and pop in O(1) instead of using iteration.
- Why do we leave one slot empty in a circular buffer?
    One slot is left empty in the buffer to distinguish between a full buffer and an empty one.
    We indicate that if head = tail the buffer is full, so if we filled up all the buffer slots it would start overwriting the index. 
    instead we have an empty spot which when the head is at, we can preemptivly check if the next head position would lead
    to a collision preventing overwriting data. 
- What is the benefit of using modulo arithmetic?
    The benifit of mudulo arithmetic is it allows the index to wrap arround after reaching the end of the array. 

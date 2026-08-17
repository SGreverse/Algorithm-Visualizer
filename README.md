# Multithreaded Algorithm Visualizer

A high-performance, interactive algorithm visualizer written in C using Raylib. 

Rather than relying on basic blocking loops, this project is built on a custom producer-consumer threading model. The UI and rendering run cleanly on the main thread, while the algorithms execute on an isolated background worker thread. They communicate via standardized VTable hooks (`compare`, `swap`, `write`) backed by mutexes and condition variables. 

This architecture allows for real-time visual updates, live operation counting, and procedural audio synthesis without ever bottlenecking the render loop.

## What It Includes
*   **Sorting Engine:** Implementations of Bubble, Selection, Insertion, Quick, Merge, Counting, Radix, and Heap Sort.
*   **Live Analytics:** Tracks algorithmic comparisons, memory swaps, and array writes in real-time.
*   **Built-in Documentation:** An integrated UI modal detailing the mechanics, step-by-step logic, and Big-O complexity (explicitly separating auxiliary heap memory from call-stack memory) for each algorithm.

## How to Use

Once compiled and running, the entire application is controlled via the UI:

*   **Algorithm Navigation:** Click the tabs at the top to hot-swap between different algorithms.
*   **Execution Controls:** Use the **START**, **STOP**, and **RESET** buttons to control the worker thread. (The thread can be safely killed mid-execution without memory leaks).
*   **Dynamic Resizing:** Use the bottom slider to dynamically adjust the array size (from 10 up to 1,000 elements). 
*   **Documentation:** Click the **DOCS** button to overlay the technical breakdown of the currently selected algorithm. 

*Note: Navigation and resizing are safely locked while an algorithm is actively running to prevent data races and memory corruption.*

## Dependencies & Building
*   **Graphics:** [Raylib](https://www.raylib.com/)
*   **Standard Libraries:** `<threads.h>`, `<stdatomic.h>`, `<math.h>`

Compile using your preferred C compiler, ensuring you link Raylib, the math library (`-lm`), and threading (`-pthread` on Linux/macOS). If running on Linux and audio fails to initialize, ensure `libasound2-plugins` is installed.

## What's Next
The VTable architecture and documentation structs were designed to be generic and algorithm-agnostic. Future updates will expand the engine beyond sorting to include:
*   **Graph Algorithms:** Visualizations for pathfinding (A*, Dijkstra) and graph traversals (BFS, DFS).
*   **Generic Algorithms:** String matching algorithms and other standard computer science data processing techniques.

---
*Developed by Segev Kam*

# threadpool-demo
## This project shows how you can use thread pools for multi-threaded data processing.
### The application reads the input-file in parts, calculates the checksum of each part and writes to the output-file
Simply run next commands to compile app:
```
cmake .
make
```
Run app:
./signature <input-file> <output-file> <block-size(optional)>

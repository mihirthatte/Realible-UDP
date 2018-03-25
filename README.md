***Realibale UDP protocol***

## To run the Application, first run make command which will compile and create executable files of client and server applications
``` make ```

## Execute server - 
cd into Server directory and server executable file.
```
cd Server
./server /server [PORT] [Receive Window] 
```
## Example Server Command - 
To execute server on PORT 20001 with window size of 20, run - 
```
./server 20001 20
```

## Execute Client - 
cd into Client directory and run the client executable file.
```
cd Client
./client [HOSTNAME] [PORT] [FILE NAME] [Receive Window] [Drop Probability] [Processing Delay]
```
## Example Client Command - 
To execute client on Hostmachine - burrow.soic.indiana.edu, running on PORT 20001, requesting file sample.txt with 0% drop probability and no processing delay.
```
./client burrow.soic.indiana.edu 20001 sample9.txt 20 0 0
```

To execute client on Hostmachine - burrow.soic.indiana.edu, running on PORT 20001, requesting file sample.txt with 10% drop probability and no processing delay
```
./client burrow.soic.indiana.edu 20001 sample9.txt 20 10 0
```


To execute client on Hostmachine - burrow.soic.indiana.edu, running on PORT 20001, requesting file sample.txt with 0% drop probability and processing delay
```
./client burrow.soic.indiana.edu 20001 sample9.txt 20 0 1
```

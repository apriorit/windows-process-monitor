# Windows Process Monitoring and Management Tool

## About

This project is a demonstration of a set of process monitoring and management techniques used mainly in various security applications.

It is a Windows process monitoring tool, which includes a driver to monitor process start. This driver collects and reports process details to user mode and can allow or block its start. 

## Project Structure

|-> .\ Common 				- Common files that are common for user and driver

|-> .\ ProcessDll           - Main DLL that has all user mode API

|-> .\ ProcessMonitor       - Process monitoring driver project

|-> .\ ProcMonGUI           - GUI that is written using MFC


## Implementation

x64/x86 architectures are supported.

Please note that the provided code only illustrates the process blocking technique and cannot be used in a commercial solution as-is.

You can find step-by-step code explanation and technology details in the [related article] (https://www.apriorit.com/dev-blog/254-monitoring-windows-processes).

## License

Licensed under the MIT license. © Apriorit.

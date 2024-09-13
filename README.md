# ReverseEngineerDrivers

Since,There are very few open source code samples for windows driver.So, I thought to Reverse engineer them and add them to this github repo . I will add all the drivers reverse engineered by me which are less than 50Kb , because reverse engineering drivers more than this size are much time taking even when they ar third party Drivers, where we have to reverse the windows wdk libraries functions also. 

- I will Reverse engineer mostly third party windows party and sometimes microsoft drivers.
- I will be using Ida pro 7.6 .
- All drivers samples and IDA database are also added.
- All the function are named after reverse engineer by me , not official.

1. [Easeuse Partition Manager](https://www.easeus.com/partition-manager/) :
- **EUEDKEPM.sys**  - It is the disk Access Driver of the partition manager.
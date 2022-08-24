Input: The input of each process is a single partition which includes a mutually exclusive subset of all nodes in the graph and their associated edges. The internal edges are between two nodes in the same partition. Each external edge is represented with an internal node in the partition and its external neighbor along with its corresponding partition. Each process also needs to know the number of partitions, their assignment to different processes, and the address of individual processes.
You can find the following input files

fl_compact.tab provides the edge representation of the entire Flickr graph as you have seen in part I.
fl_compact_part.2 and fl_compact_part.4 present the partitioning of all nodes in the Flickr graph into 2 and 4 partitions, respectively. Each one of these files has a separate line for each node that provides the following information : 1) Node ID, 2) Node Degree, 3) Node Partition ID . Note that partition ID starts from 0.
To run your code with n processes, you should provide two files, namely fl_compact.tab and fl_compact_part.n, to each process. Each process should use this information to label individual nodes by their corresponding partitions. This in turn shows whether an edge is internal or external which is essential for managing the update of node credits correctly.

To Run:'programName graphFile partitionFile rounds'



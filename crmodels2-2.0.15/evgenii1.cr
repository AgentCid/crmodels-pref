#domain vertex(X).
#domain vertex(Y).
vertex(1..30). % vertices 
edge_in_graph(1, 2). % edge in the graph
edge_in_graph(2, 30).  % edge in the graph
edge_in_graph(1, 30).  % edge in the graph
% rarely but if has to, there is a path between X, Y if there is an edge
% between them. 
r(X, Y): edge_in_path(X,Y) +- edge_in_graph(X,Y),vertex(X),vertex(Y).
connected(X,Y):- edge_in_path(X,Y).
connected(X,Z):- edge_in_path(X,Y),connected(Y,Z).
% :- not connected(1,30). % find a shortest path between 1 and 30. 
% #hide.
#hide vertex(X).
% #show connected(X, Y). 
% #show edge_in_path(X,Y).

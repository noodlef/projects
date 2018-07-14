#include<iostream>
#include<string>
#include<vector>
#include<deque>

using namespace std;
struct pos {
	int x;
	int y;
	pos(int i, int j)
	{ 
		x = i;
		y = j;
	}
	pos(){}
};
int main() {
	int n;
	cin >> n;
	vector<vector<char>> s_map;
	vector<vector<int>> dis;
	pos source;
	for (size_t i = 0; i < n; i++) {
		vector<char> row;
		for (size_t j = 0; j < n; j++) {
			char c;
			cin >> c;
			if (c == '*') {
				source.x = i;
				source.y = j;
			}
			row.push_back(c);
		}
		s_map.push_back(row);
		dis.push_back(vector<int>(n, -1));
	}
	// bfs
	deque<pos> que;
	que.push_front(source);
	dis[source.x][source.y] = -2;
	int steps = 0;
	int cnt = 1, tcnt = 0;
	while (que.size()) {
		pos position = que.front();
		que.pop_front();
		--cnt;
		vector<pos> points;
		points.push_back(pos(position.x - 1, position.y));
		points.push_back(pos(position.x + 1, position.y));
		points.push_back(pos(position.x, position.y + 1));
		points.push_back(pos(position.x, position.y - 1));
		for (size_t i = 0; i < 4; i++) {
			if (points[i].x < 0 || points[i].x >= n)
				continue;
			if (points[i].y < 0 || points[i].y >= n)
				continue;
			if (s_map[points[i].x][points[i].y] == '#')
				continue;
			if (dis[points[i].x][points[i].y] == -1) {
				dis[points[i].x][points[i].y] = -2;
				que.push_back(points[i]);
				++tcnt;
			}
		}
		if (s_map[position.x][position.y] == '@')
			break;
		if (cnt == 0) {
			cnt = tcnt;
			tcnt = 0;
			++steps;
		}
	}
	cout << steps << endl;
	system("pause");
}

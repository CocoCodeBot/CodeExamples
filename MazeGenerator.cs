using System.IO;
using UnityEngine;

public class MazeGenerator : MonoBehaviour
{
    [SerializeField] private GameObject wall;

    [SerializeField] private int width = 30;
    [SerializeField] private int height = 30;
    [SerializeField] private float cellSize = 6f; // Cell side length

    public int[,] maze;
    private bool[,] visited;

    private const int FullCellWalls = 15; // 1111 in binary
    private const int DirCount = 4; // Number of directions

    // {m1, m2, angle}
    // First two are multipliers to calculate cell corner position for given (x,y) coordinates.
    // Third is an Y-axis angle of the wall which will be created in that corner.
    private static readonly int[,] orientationInfo = {
        {1, 1, 270}, // Top wall
        {1, 1, 180}, // Right wall
        {-1, -1, 90}, // Bottom wall
        {-1, -1, 0} // Left wall
    };

    private void Update()
    {
        // Just for development
        if (Input.GetKeyDown(KeyCode.T))
        {
            GenerateMaze();
            DrawMaze();

            SaveMaze("SavedMaze/maze.txt");
        }
    }

    // This function saves last generated
    // maze info to a file.
    private void SaveMaze(string filename)
    {
        string path = Path.Combine(Application.dataPath, filename);

        using (StreamWriter writer = new StreamWriter(path))
        {
            width = maze.GetLength(0);
            height = maze.GetLength(1);

            writer.WriteLine(width);
            writer.WriteLine(height);

            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    writer.WriteLine(maze[i, j]);
                }
            }

            writer.Close();
        }
    }

    public void LoadMaze(string filename)
    {
        string path = Path.Combine(Application.dataPath, filename);

        using (StreamReader reader = new StreamReader(path))
        {
            string line = reader.ReadLine();
            int rows = int.Parse(line);
            line = reader.ReadLine();
            int cols = int.Parse(line);

            maze = new int[rows, cols];

            for (int i = 0; i < rows; i++)
            {
                for (int j = 0; j < cols; j++)
                {
                    line = reader.ReadLine();
                    maze[i, j] = int.Parse(line);
                }
            }

            reader.Close();
        }
    }

    private void GenerateFullMaze()
    {
        maze = new int[width, height];
        visited = new bool[width, height];

        // Initialize maze with walls.
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                maze[x, y] = FullCellWalls;
            }
        }
    }

    // Wall info for a given cell is kept in binary (0-15 = 0000-1111).
    // Set n-th bit to 0: x AND (NOT 2^n)
    // Set n-th bit to 1: x OR 2^n
    // Check if n-th bit is set: x AND 2^n != 0
    // Where: 2^n = 1 << n, AND = &, OR = |, NOT = ~
    private void GenerateMaze()
    {
        maze = new int[width, height];
        visited = new bool[width, height];

        // Initialize maze with walls.
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                maze[x, y] = FullCellWalls;
            }
        }

        // Generate maze
        DFS(0, 0);
    }

    // It is a randomized DFS for a maze generation.
    private void DFS(int x, int y)
    {
        visited[x, y] = true;

        // {x, y, dirID}
        // Representation of four directions in classic coordinate system.
        int[][] directions = {
            new int[]{ 0, 1, 0 }, // Up
            new int[]{ 1, 0, 1 }, // Right
            new int[]{ 0, -1, 2 }, // Down
            new int[]{ -1, 0, 3 } // Left
        };

        ShuffleDirections(directions, 4);

        // Recursive DFS calls for every direction.
        for (int i = 0; i < DirCount; i++)
        {
            int nextX = x + directions[i][0];
            int nextY = y + directions[i][1];

            if (IsInBounds(nextX, nextY))
            {
                if (!visited[nextX, nextY])
                {
                    int currentDirID = directions[i][2];

                    // A formula below translates an ID of a direction
                    // through which the DFS travels to an ID of a direction
                    // from which the DFS has arrived.
                    int nextDirID = (currentDirID + 2) % 4;

                    // Zeroing proper bits of both current and next cell.
                    maze[x, y] &= ~(1 << currentDirID);
                    maze[nextX, nextY] &= ~(1 << nextDirID);

                    DFS(nextX, nextY);
                }
                else
                {
                    int chance = Random.Range(1, 101);

                    if (chance <= 5)
                    {
                        int currentDirID = directions[i][2];

                        // A formula below translates an ID of a direction
                        // through which the DFS travels to an ID of a direction
                        // from which the DFS has arrived.
                        int nextDirID = (currentDirID + 2) % 4;

                        // Zeroing proper bits of both current and next cell.
                        maze[x, y] &= ~(1 << currentDirID);
                        maze[nextX, nextY] &= ~(1 << nextDirID);
                    }
                }
            }
        }
    }

    private void ShuffleDirections(int[][] directions, int swapCount)
    {
        for (int i = 0; i < swapCount; i++)
        {
            int randomFirst = Random.Range(0, DirCount);
            int randomSecond = Random.Range(0, DirCount);
            int[] temp = directions[randomFirst];

            directions[randomFirst] = directions[randomSecond];
            directions[randomSecond] = temp;
        }
    }

    public bool IsInBounds(int x, int y)
    {
        return x >= 0 && y >= 0 && x < width && y < height;
    }

    public void DrawMaze()
    {
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                // Checking every bit of a cell to draw walls.
                // Drawing only right and top wall except when x or y equals 0.
                // This is to avoid printing two walls in one place.
                for (int i = 0; i < DirCount; i++)
                {
                    if ((maze[x, y] & 1 << i) != 0)
                    {
                        // Check if there is no need to print left or bottom wall.
                        if ((i == 2 && y != 0) || (i == 3 && x != 0))
                            continue;

                        // Pivot coordinates for the new wall.
                        float pivotX = transform.position.x + cellSize * (x + orientationInfo[i, 0] * 0.5f);
                        float pivotZ = transform.position.z + cellSize * (y + orientationInfo[i, 1] * 0.5f);

                        // Position and rotation for the new wall.
                        Vector3 pos = new Vector3(pivotX, transform.position.y, pivotZ);
                        Quaternion rot = Quaternion.Euler(0, orientationInfo[i, 2], 0);

                        // Creating the wall.
                        Instantiate(wall, pos, rot, transform);
                    }
                }
            }
        }
    }
}

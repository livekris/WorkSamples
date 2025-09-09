using System;
using System.Net.Http;
using System.Net.Http.Json;
using System.Threading.Tasks;

namespace Tests
{
    public static class IntegrationTest
    {
        private static readonly HttpClient client = new HttpClient
        {
            BaseAddress = new Uri("http://localhost:5000")
        };

        public static async Task RunAllTestsAsync()
        {
            Console.WriteLine("Running server tests...");

            await PlayerEntersCasino();

            Console.WriteLine("All tests completed.");
        }

        private static async Task PlayerEntersCasino()
        {
            var names = new string[] { "Alice", "Bob", "Eve", "Mallory", "Trent", "Peggy" };
            var random = new Random();

            int max = 2000;
            int count = 0;

            while (true)
            {
                if (count > max)
                    return;

                string randomName = names[random.Next(names.Length)];

                string[] Endpoints = new string[3];

                // Assign values
                Endpoints[0] = $"/players/enter?name={randomName}";
                Endpoints[1] = $"/players/play?name={randomName}&bet=1";
                Endpoints[2] = $"/players/leave?name={randomName}";
                var randEndpoints = new string[] { Endpoints[0], Endpoints[1], Endpoints[2] };
                string randomEndpoint = randEndpoints[random.Next(randEndpoints.Length)];

                for (int i = 0; i < 3; i++)
                {
                    var response = await client.PostAsync(randomEndpoint, null);

                    if (response.IsSuccessStatusCode)
                    {
                        var content = await response.Content.ReadAsStringAsync();
                        Console.WriteLine($"{randomName}:, {content}");
                    }
                    else
                    {
                        Console.WriteLine($"{randomName} failed entering: {response.StatusCode}");
                    }
                }

                count++;
            }          
        }
    }

    public class SlotResult
    {
        public int[] Slots { get; set; } = new int[3];
        public int Winnings { get; set; }
        public int Balance { get; set; }
    }
}

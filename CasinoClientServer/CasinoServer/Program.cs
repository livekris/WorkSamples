using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Hosting;
using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Runtime.Intrinsics.Arm;
using System.Text.Json;
using LiteDB;
using Model;

var builder = WebApplication.CreateBuilder(args);
var app = builder.Build();

bool runTests = args.Contains("--test");

// To avoid creating multiple projects, we wil test the casino server with two instances of CasinoServer app
// one on 5000 port and the test one on 5001 port: dotnet run --project CasinoServer "http://localhost:5001" -- --test
if (runTests)
{
    Console.WriteLine("Server running in TEST MODE...");

    // Run tests and wait for completion
    await Tests.IntegrationTest.RunAllTestsAsync();

    Console.WriteLine("Tests completed. Exiting.");
    return; // Exit the application after tests
}

// Thread-safe database for player records
using var database = new PlayerDatabase();

app.MapGet("/health", () => Results.Ok("Server is running"));

// Player enters the casino
app.MapPost("/players/enter", (string name) =>
{
    if (string.IsNullOrEmpty(name))
        return Results.BadRequest("Player name required.");

    if (database.GetPlayer(name) is null)
    {
        database.Update(new Player { Name = name });
        return Results.Ok($"{name} entered the casino with $10 balance.");
    }

    return Results.Ok($"{name} is already in the casino.");
});

// Player plays a slot machine
app.MapPost("/players/play", (string name, int bet = 1) =>
{
    var player = database.GetPlayer(name);
    if (player is null)
        return Results.BadRequest("Player not found.");
    if (player.Balance <= 0 || bet > player.Balance)
        return Results.BadRequest("Insufficient balance.");

    int winnings = 0;
    int[] slot = Model.SlotMachine.PlaySlots(player, out winnings);

    if (winnings > 0)
        player.Credit(winnings);
    else
        player.Debit(bet);

    // save the new balance for the player
    database.Update(player);

    return Results.Ok(new
    {
        Slots = new[] { slot[0], slot[1], slot[2] },
        Balance = player.Balance
    });
});

// Player leaves casino
app.MapPost("/players/leave", (string name) =>
{
    var player = database.GetPlayer(name);
    if (player is null)
        return Results.NotFound("Player not found.");
    if (player.Balance > 0)
        return Results.Ok(
        $"{name} has left the casino and may return to play with the remaining ${player.Balance} credit.");

    database.Remove(name);

    return Results.Ok($"{name} has left the casino.");
});

// Player checks balance
app.MapGet("/players/balance", (string name) =>
{
    var player = database.GetPlayer(name);
    if (player is null)
        return Results.BadRequest("Player not found.");
    return Results.Ok(player.Balance);
});

// Player cashes out
app.MapPost("/players/cashout", (string name) =>
{
    var player = database.GetPlayer(name);
    if (player is null)
        return Results.BadRequest("Player not found.");

    var oldBalance = player.Balance;
    player.Cashout();
    database.Remove(player.Name);

    return Results.Ok(oldBalance);
});

// Run server on HTTP only
app.Run("http://localhost:5000");
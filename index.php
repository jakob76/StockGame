<?php
session_start();
$portfolio = $_SESSION['portfolio'] ?? [];
$cash = $_SESSION['cash'] ?? 5000.0;
$saveFile = $_SESSION['saveFile'] ?? '';

function run_backend($action, $ticker = '', $shares = 0) {
    $ticker = strtoupper(trim($ticker));
    $cmd = escapeshellcmd("./stock_game_backend $action $ticker $shares");
    $output = shell_exec($cmd);
    return !empty($output) ? json_decode($output, true) : ["error" => "No output from backend."];
}

// Load save file
if (isset($_POST['load'])) {
    $filename = $_POST['load_file'];
    if (file_exists($filename)) {
        $data = json_decode(file_get_contents($filename), true);
        $_SESSION['cash'] = $cash = $data['cash'];
        $_SESSION['portfolio'] = $portfolio = $data['portfolio'];
        $_SESSION['saveFile'] = $saveFile = $filename;
        $message = "Loaded from $filename";
    } else {
        $message = "Save file not found.";
    }
}

// Save and exit
if (isset($_POST['exit'])) {
    $filename = $saveFile ?: "portfolio_save.json";
    $data = ['cash' => $cash, 'portfolio' => $portfolio];
    file_put_contents($filename, json_encode($data, JSON_PRETTY_PRINT));
    session_destroy();
    $message = "Portfolio saved to $filename. Game exited.";
}

// Stock price lookup
if (isset($_POST['lookup'])) {
    $ticker = $_POST['ticker'];
    $result = run_backend("price", $ticker);
}

// Buy stocks
if (isset($_POST['buy'])) {
    $ticker = $_POST['ticker'];
    $shares = intval($_POST['shares']);
    $info = run_backend("price", $ticker);
    if (!isset($info['error']) && $info['price'] > 0) {
        $cost = $info['price'] * $shares;
        if ($cost <= $cash) {
            $cash -= $cost;
            if (isset($portfolio[$ticker])) {
                $oldShares = $portfolio[$ticker][0];
                $oldAvg = $portfolio[$ticker][1];
                $newShares = $oldShares + $shares;
                $newAvg = (($oldShares * $oldAvg) + $cost) / $newShares;
                $portfolio[$ticker] = [$newShares, $newAvg];
            } else {
                $portfolio[$ticker] = [$shares, $info['price']];
            }
            $_SESSION['cash'] = $cash;
            $_SESSION['portfolio'] = $portfolio;
            $message = "Bought $shares shares of $ticker.";
        } else {
            $message = "Insufficient cash.";
        }
    } else {
        $message = $info['error'] ?? "Failed to fetch price.";
    }
}

// Sell stocks
if (isset($_POST['sell'])) {
    $ticker = $_POST['ticker'];
    $shares = intval($_POST['shares']);
    if (!isset($portfolio[$ticker])) {
        $message = "You don't own $ticker.";
    } elseif ($shares > $portfolio[$ticker][0]) {
        $message = "Not enough shares to sell.";
    } else {
        $info = run_backend("price", $ticker);
        if (!isset($info['error']) && $info['price'] > 0) {
            $revenue = $info['price'] * $shares;
            $cash += $revenue;
            $portfolio[$ticker][0] -= $shares;
            if ($portfolio[$ticker][0] === 0) unset($portfolio[$ticker]);
            $_SESSION['cash'] = $cash;
            $_SESSION['portfolio'] = $portfolio;
            $message = "Sold $shares shares of $ticker.";
        } else {
            $message = $info['error'] ?? "Failed to fetch price.";
        }
    }
}
?>

<!DOCTYPE html>
<html>
<head><title>Stock Market Game</title></head>
<body>
<h1>Stock Market Game</h1>

<?php if (!empty($message)) echo "<p><strong>$message</strong></p>"; ?>
<?php if (isset($result) && !isset($result['error'])): ?>
    <p>Ticker: <?= htmlspecialchars($result['ticker']) ?></p>
    <p>Price: $<?= number_format($result['price'], 2) ?></p>
    <p>Date: <?= htmlspecialchars($result['date']) ?></p>
<?php elseif (isset($result['error'])): ?>
    <p style="color:red;">Error: <?= htmlspecialchars($result['error']) ?></p>
<?php endif; ?>

<form method="POST">
    <label>Load Save File:</label>
    <input type="text" name="load_file" placeholder="filename.json">
    <button name="load">Load</button>
</form>

<form method="POST">
    <label>Stock Ticker:</label>
    <input type="text" name="ticker" required>
    <button name="lookup">Lookup</button>
</form>

<form method="POST">
    <label>Ticker:</label>
    <input type="text" name="ticker" required>
    <label>Shares to Buy:</label>
    <input type="number" name="shares" required>
    <button name="buy">Buy</button>
</form>

<form method="POST">
    <label>Ticker:</label>
    <input type="text" name="ticker" required>
    <label>Shares to Sell:</label>
    <input type="number" name="shares" required>
    <button name="sell">Sell</button>
</form>

<form method="POST">
    <button name="exit">Save and Exit</button>
</form>

<h2>Your Portfolio</h2>
<p>Cash: $<?= number_format($cash, 2) ?></p>
<?php if (empty($portfolio)): ?>
    <p>You don't own any stocks.</p>
<?php else: ?>
    <table border="1" cellpadding="5">
        <tr><th>Ticker</th><th>Shares</th><th>Avg. Price</th></tr>
        <?php foreach ($portfolio as $ticker => $data): ?>
            <tr>
                <td><?= htmlspecialchars($ticker) ?></td>
                <td><?= $data[0] ?></td>
                <td>$<?= number_format($data[1], 2) ?></td>
            </tr>
        <?php endforeach; ?>
    </table>
<?php endif; ?>
</body>
</html>

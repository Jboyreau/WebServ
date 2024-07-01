<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Submit Comment</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            margin: 0;
            padding: 0;
        }
        .container {
            width: 60%;
            margin: auto;
            overflow: hidden;
        }
        #main {
            background: #fff;
            color: #333;
            padding: 20px;
            margin-top: 30px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        form {
            display: flex;
            flex-direction: column;
        }
        input[type="text"], textarea {
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 5px;
            font-size: 16px;
        }
        input[type="submit"] {
            padding: 10px;
            border: none;
            background: #5cb85c;
            color: white;
            font-size: 16px;
            border-radius: 5px;
            cursor: pointer;
        }
        input[type="submit"]:hover {
            background: #4cae4c;
        }
        .comments {
            margin-top: 20px;
        }
        .comment {
            background: #fff;
            padding: 10px;
            margin-bottom: 10px;
            border-radius: 5px;
            box-shadow: 0 0 5px rgba(0, 0, 0, 0.1);
        }
        .comment h4 {
            margin: 0;
            padding-bottom: 5px;
            border-bottom: 1px solid #ddd;
        }
        .comment p {
            margin: 0;
            padding: 10px 0 0 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div id="main">
            <h1>Submit Your Comment</h1>
            <form action="submit_comment.php" method="post">
                <input type="text" name="name" placeholder="Your Name" required>
                <textarea name="comment" placeholder="Your Comment" required></textarea>
                <input type="submit" value="Submit Comment">
            </form>

            <div class="comments">
                <h2>Comments</h2>
                <?php
                // Simple comment submission script
                if ($_SERVER['REQUEST_METHOD'] == 'POST') {
                    $name = htmlspecialchars($_POST['name'], ENT_QUOTES, 'UTF-8');
                    $comment = htmlspecialchars($_POST['comment'], ENT_QUOTES, 'UTF-8');

                    // Append comment to comments.txt
                    $file = fopen('comments.txt', 'a');
                    if ($file) {
                        fwrite($file, "<div class='comment'><h4>$name</h4><p>$comment</p></div>\n");
                        fclose($file);
                    } else {
                        echo "<p>Unable to open comments file for writing.</p>";
                    }
                }

                // Display comments
                if (file_exists('comments.txt')) {
                    echo file_get_contents('comments.txt');
                }
                ?>
            </div>
        </div>
    </div>
</body>
</html>

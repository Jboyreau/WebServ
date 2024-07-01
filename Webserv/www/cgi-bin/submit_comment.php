<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Video and Comments</title>
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
        video {
            width: 100%;
            height: auto;
            border-radius: 8px;
        }
        form {
            display: flex;
            flex-direction: column;
            margin-top: 20px;
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
            <h1>Watch the Video and Comment Below</h1>
            <?php
            if (isset($_GET['video'])) {
                $video = htmlspecialchars($_GET['video']);
                echo "<video controls>
                        <source src='$video' type='video/mp4'>
                        Your browser does not support the video tag.
                      </video>";
            } else {
                echo "<p>No video selected.</p>";
            }
            ?>

            <form action="submit_comment.php?video=<?php echo urlencode($video); ?>" method="post">
                <input type="text" name="name" placeholder="Your Name" required>
                <textarea name="comment" placeholder="Your Comment" required></textarea>
                <input type="submit" value="Submit Comment">
            </form>

            <div class="comments">
                <h2>Comments</h2>
                <?php
                if ($_SERVER['REQUEST_METHOD'] == 'POST' && isset($_GET['video'])) {
                    $name = htmlspecialchars($_POST['name']);
                    $comment = htmlspecialchars($_POST['comment']);
                    $video = htmlspecialchars($_GET['video']);

                    // Store comments in a file specific to the video
                    $comments_file = 'comments/comments_' . pathinfo($video, PATHINFO_FILENAME) . '.txt';
                    $file = fopen($comments_file, 'a');
                    fwrite($file, "<div class='comment'><h4>$name</h4><p>$comment</p></div>\n");
                    fclose($file);
                }

                // Display comments for the specific video
                if (isset($_GET['video'])) {
                    $video = htmlspecialchars($_GET['video']);
                    $comments_file = 'comments/comments_' . pathinfo($video, PATHINFO_FILENAME) . '.txt';
                    if (file_exists($comments_file)) {
                        echo file_get_contents($comments_file);
                    }
                }
                ?>
            </div>
        </div>
    </div>
</body>
</html>

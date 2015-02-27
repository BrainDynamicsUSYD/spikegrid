CREATE TABLE IF NOT EXISTS `cson_session` (
  `id` varchar(50) PRIMARY KEY NOT NULL,
  `last_saved` int(11) NOT NULL DEFAULT 0,
  `json` text DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

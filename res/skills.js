
function leaf_01(points, maxpoints) {
	var str = "Tree. Root. \n" + points + "/" + maxpoints + "\n";

	switch (points) {
		case 0:
			str += "Left click to upgrade";
			break;
		case 1:
			str += "Right click to downgrade";
			break;
		default:
			str += ":)"
	}

	str += "\n";

	return str;
}

// Grids operative data
var grids = {};
// Count of grid values to load by once
var grid_page_limit = 50;

/**
 * Drupal execute script on page load behavior
 */
Drupal.behaviors.grid = function (context) {
	// Remove "Add row" buttons
	$('a.grid-addrow').remove();
	
	// Populate grids variable
	for(gridName in Drupal.settings.mywebform.grids) {
		var fieldName = '';
		for(fieldName in Drupal.settings.mywebform.grids[gridName].fields) break;
		grids[gridName] = {
			'count' : Drupal.settings.mywebform.grids[gridName].count,
			'fields' : Drupal.settings.mywebform.grids[gridName].fields,
			'page_limit' : grid_page_limit,
			'offset' : 0,
			'defField' : fieldName
		};
	}
	
	// Load grid data
	grid_load_data();
}

/**
 * Runs code after all grid data have been loaded
 */
function grid_after_load() {
	$.each(webform.afterLoad, function() {
		this();
	});
	
	if(Drupal.settings.mywebform.print) {
		window.print();
	}
}

/**
 * Load grids' rows and fill them with values
 */
function grid_load_data() {
	// Find the next not fully loaded grid
	var gridName = '';
	for(x in grids) {
		if(grids[x].offset < grids[x].count) {
			gridName = x;
			break;
		}
	}
	
	// End function if no grid found
	if(gridName == '') {
		$('.drow').remove();
		grid_after_load();
		return false;
	}
	
	// Add rows
	var drow = $('#' + gridName + ' .drow');
	var max_index = Math.min(grids[gridName].offset + grids[gridName].page_limit, grids[gridName].count);
	for(index = grids[gridName].offset; index < max_index; index++) {
		var row = drow.clone();
		row.addClass('row');
		row.addClass('row-' + index);
		row.removeClass('drow');
		row.show();
		row.find('span.form-item').each(function (i) {
			var fieldName = $(this).attr('field');
			var id = fieldName + '-' + index;
			$(this).attr('id', id).html(Drupal.settings.mywebform.values[fieldName][index]);
		});
		
		// Insert row in table
		row.insertBefore(drow);
	
		// Update max row index
		if(index > grids[gridName].maxindex) {
			grids[gridName].maxindex = index;
		}
	}
	
	grids[gridName].offset = max_index;
	
	setTimeout('grid_load_data()', 100);
}


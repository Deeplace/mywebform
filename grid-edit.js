var grids = {};
var grid_page_limit = 50;

Drupal.behaviors.grid = function (context) {

	$('#mywebform-edit-form').submit(function () {
		// Serialize grid values before submit
		var str = JSON.stringify(Drupal.settings.mywebform.values);
		$('#serialized_data').val(str);
	});

	$('#mywebform-edit-form a.grid-addrow').click(function() {
		if(!$(this).hasClass('disabled')) {
			var gridName = $(this).attr('grid');
			grid_add_row(gridName);
		}
		return false;
	});

	$('#mywebform-edit-form td.delrow a').live('click', function(evt) {
		var gridName = $(this).attr('grid');
		var row = $(this).parent().parent();
		var index = parseInt(row.attr('row-index'));
		var indexField = '';
	    if(index < 1) {
          index = 0;
        }
		if (grids[gridName].count <= 1) {
		  return false;
		}
		row.hide();
		row.find('input, select').each(function () {
			var fieldName = $(this).attr('field');
			if($(this).hasClass('row-index')) {
				indexField = fieldName;
			}
			if(typeof fieldName != 'undefined' && typeof Drupal.settings.mywebform.fields[fieldName] != 'undefined' && Drupal.settings.mywebform.fields[fieldName].autofield.length) {
				$(this).val('').trigger('change');
			}
		});
		setTimeout("grid_remove_row('" + gridName + "', " + index + ", '" + indexField + "');", 50);
		return false;
	});

	grid_initialize();

	$('#mywebform-edit-form .scroll-start').click(function () {
		var gridName = $(this).attr('grid');
		grid_refresh(gridName, 1);
		return false;
	});

	$('#mywebform-edit-form .scroll-end').click(function () {
		var gridName = $(this).attr('grid');
		var offset = (grids[gridName].pages - 1) * grids[gridName].page_limit + 1;
		grid_refresh(gridName, offset);
		return false;
	});

	$('#mywebform-edit-form .scroll-up').click(function () {
		var gridName = $(this).attr('grid');
		var offset = Math.max(grids[gridName].offset - 1, 1);
		grid_refresh(gridName, offset);
		return false;
	});

	$('#mywebform-edit-form .scroll-down').click(function () {
		var gridName = $(this).attr('grid');
		var offset = Math.min(grids[gridName].offset + 1, grids[gridName].count);
		grid_refresh(gridName, offset);
		return false;
	});

	$('#mywebform-edit-form .scroll-pgup').click(function () {
		var gridName = $(this).attr('grid');
		var offset = Math.max(1, grids[gridName].offset - grids[gridName].page_limit);
		grid_refresh(gridName, offset);
		return false;
	});

	$('#mywebform-edit-form .scroll-pgdn').click(function () {
		var gridName = $(this).attr('grid');
		var offset = Math.min(grids[gridName].count, grids[gridName].offset + grids[gridName].page_limit);
		grid_refresh(gridName, offset);
		return false;
	});

	$('#mywebform-edit-form .page-jump').live('click', function () {
		var gridName = $(this).attr('grid');
		var page = parseInt($(this).attr('page'));
		var offset = (page - 1) * grids[gridName].page_limit + 1;
		grid_refresh(gridName, offset);
		return false;
	});
}

/**
 * Initialize all grids
 */
function grid_initialize() {
	$('a.grid-addrow').addClass('disabled');

	for(gridName in Drupal.settings.mywebform.grids) {
		// Initialize grid's parameters
		grids[gridName] = {
			'count' : Drupal.settings.mywebform.grids[gridName].count,
			'fields' : Drupal.settings.mywebform.grids[gridName].fields,
			'page_limit' : grid_page_limit,
			'offset' : 1,
			'defField' : '',
			'pages' : 0,
			'curPage' : 0
		};

		// Set grid's first field
		var fieldName = '';
		for(x in grids[gridName].fields) {
			fieldName = grids[gridName].fields[x];
			grids[gridName].defField = fieldName;
			break;
		}

		// Insert grid's scroll buttons and pages
		var str = '';
		str += '<div class="grid-scroll">';
		str += '<ul class="scroll">';
		str += '<li><a href="#" grid="' + gridName + '" class="scroll-start">Start</a></li>';
		str += '<li><a href="#" grid="' + gridName + '" class="scroll-pgup">Page up</a></li>';
		str += '<li><a href="#" grid="' + gridName + '" class="scroll-up">Line up</a></li>';
		str += '<li><a href="#" grid="' + gridName + '" class="scroll-down">Line down</a></li>';
		str += '<li><a href="#" grid="' + gridName + '" class="scroll-pgdn">Page down</a></li>';
		str += '<li><a href="#" grid="' + gridName + '" class="scroll-end">End</a></li>';
		str += '</ul>';
		str += '<ul class="pages"></ul>';
		str += '<div style="clear: both;"></div>';
		str += '</div>';
		$('#' + gridName).prepend(str);
		grid_update_pager(gridName);

		// Refresh grid
		if(grids[gridName].count) {
			grid_refresh(gridName, 1);
		} else {
			// Delayed add row execution to wait while all event handlers will be appended
			setTimeout("$('#" + gridName + " a.grid-addrow').trigger('click');", 10);
		}

		$('#' + gridName + ' a.grid-addrow').attr('grid', gridName);
		$('#' + gridName + ' td.delrow a').attr('grid', gridName);

        if(!Drupal.settings.mywebform.preview) {
          for (x in Drupal.settings.mywebform.fields) {
            var field = Drupal.settings.mywebform.fields[x];
            if (field.grid_name != '' && field.error !== null) {
              for (y in field.error) {
                var i = parseInt((parseInt(y) + 1) / 50) + 1;
                if (!$('#mywebform-edit-form .page-jump.page-' + i).hasClass('error')) {
                  $('#mywebform-edit-form .page-jump.page-' + i).addClass('error');
                }
              }
            }
          }
        }
	}

	$('a.grid-addrow').removeClass('disabled');

	setTimeout('$.each(webform.afterLoad, function() {this();});', 10);

	if(Drupal.settings.mywebform.print) {
		window.print();
	}
}

/**
 * Update grid's pager
 */
function grid_update_pager(gridName) {
	var pages = Math.max(1, Math.ceil(grids[gridName].count / grids[gridName].page_limit));
	var curPage = Math.floor(grids[gridName].offset / grids[gridName].page_limit) + 1;

	if(pages != grids[gridName].pages) {
		grids[gridName].pages = pages;
		grids[gridName].curPage = 0;

		var str = '';
		for(var x = 1; x <= pages; x++) {
			str += '<li><a href="#" grid="' + gridName + '" class="page-jump page-' + x + '" page="' + x + '">' + x + '</a></li>';
		}
		$('#' + gridName + ' ul.pages').html(str);

		if(pages == 1) {
			$('#' + gridName + ' .grid-scroll').hide();
		} else {
			$('#' + gridName + ' .grid-scroll').show();
		}
	}

	if(curPage != grids[gridName].curPage) {
		grids[gridName].curPage = curPage;
		$('#' + gridName + ' .pages .page-jump').removeClass('active');
		$('#' + gridName + ' .pages .page-' + curPage).addClass('active');
	}
}

/**
 * Adds a row to the end of the grid
 */
function grid_add_row(gridName) {
	var indexField = $('#' + gridName + ' input.row-index').attr('field');

	if(indexField) {
		var maxIndex = parseInt(Drupal.settings.mywebform.values[indexField][Drupal.settings.mywebform.values[indexField].length - 1]);
        $('#' + gridName + ' input.row-index').trigger('change');
	} else {
		maxIndex = 0;
        $('#' + gridName + ' input.row-index').trigger('change');
	}
	if(isNaN(maxIndex)) maxIndex = 0;
	maxIndex++;

	var fileFieldValue = [];
	$('#' + gridName + ' input[type=file][row-index].no-empty').each(function () {
		fileFieldValue[$(this).attr('row-index')] = {
			'file': $(this).val(),
		};
	});
	if (fileFieldValue.length >= 1) {
		$('#' + gridName + ' input[type=hidden][row-index]').each(function () {
			fileFieldValue[$(this).attr('row-index')].hidden = $(this).val();
		});
		for(var i in fileFieldValue) {
			if (fileFieldValue[i].file === '' && fileFieldValue[i].hidden === '') {
				return;
			}
		}
	}

	for(x in grids[gridName].fields) {
		var fieldName = grids[gridName].fields[x];
		if(fieldName == indexField) {
			Drupal.settings.mywebform.values[fieldName].push(maxIndex);
		} else {
			Drupal.settings.mywebform.values[fieldName].push($('#' + fieldName).val());
		}
	}

	grids[gridName].count++;
	$('#' + gridName + ' .scroll-end').trigger('click');
    $('#' + gridName).trigger('row_added');
}

/**
 * Removes a specified row from the grid
 */
function grid_remove_row(gridName, index, indexField) {
	var row = $('#' + gridName + ' .row-' + index);
	row.remove();

	for(x in grids[gridName].fields) {
		var fieldName = grids[gridName].fields[x];
        Drupal.settings.mywebform.values[fieldName].splice(index - 1, 1);
	}

	grids[gridName].count--;

    if(grids[gridName].count < 1) {
      grids[gridName].count = 0;
    }

    if (grids[gridName].offset > grids[gridName].count) {
      grids[gridName].offset -= grids[gridName].page_limit;
    }

	var offset = Math.max(0, Math.min(grids[gridName].offset, grids[gridName].count));

	if(indexField != '') {
		for(x in Drupal.settings.mywebform.values[indexField]) {
			if(parseInt(x) >= index - 1) {
				Drupal.settings.mywebform.values[indexField][x] = parseInt(Drupal.settings.mywebform.values[indexField][x]) - 1;
			}
		}
	}

	// Reorder rows after current
	$('#' + gridName + ' .row').each(function () {
		i = parseInt($(this).attr('row-index'));
		if(i > index) {
			$(this).removeClass('row-' + i);
			i--;
			$(this).addClass('row-' + i);
			$(this).attr('row-index', i);
			$(this).find('input, select').each(function () {
				var fieldName = $(this).attr('field');
				$(this).attr('name', fieldName + '[' + i + ']');
				$(this).attr('id', fieldName + '-' + i);
			});
			$(this).find('input, select').attr('row-index', i);
		}
	    if(indexField) {
	    	$(this).find('input.row-index').val(Drupal.settings.mywebform.values[indexField][i - 1]).trigger('change');
	    }
	});

	// Reindex errors
	for(x in webform.errors) {
		if(Drupal.settings.mywebform.fields[webform.errors[x].fieldName].grid_name == gridName) {
			if(webform.errors[x].index == index - 1) {
				webform.errors.splice(x, 1)
			} else if(webform.errors[x].index >= index) {
				webform.errors[x].index--;
			}
		}
	}

	grid_refresh(gridName, offset);
}

/**
 * Refreshes the grid rows
 */
function grid_refresh(gridName, offset) {
	grid_remove_rows(gridName, offset);

	var drow_original = $('#' + gridName + ' tr.drow');
	var drow = drow_original.clone();
	drow.removeClass('drow').show();
	drow.find('input, select').addClass('unprocessed');

	// Prepend new rows
	if(offset < grids[gridName].offset) {
		for(index = grids[gridName].offset - 1; index >= offset; index--) {
			// Prepend row
			var row = drow.clone();
			row.addClass('row row-' + index);
			row.attr('row-index', index);
			row.find('input, select, span.form-item').each(function () {
				var fieldName = $(this).attr('field');
				$(this).attr('row-index', index);
				$(this).attr('name', fieldName + '[' + index + ']');
				$(this).attr('id', fieldName + '-' + index);
			});

			// Insert row in table
			row.insertBefore($('#' + gridName + ' .row-' + (index + 1)));
		}
	}
	grids[gridName].offset = offset;

	// Append new rows
	var row_count = $('#' + gridName + ' tr.row').length;
	var max_index = Math.min(grids[gridName].offset + grids[gridName].page_limit - 1, grids[gridName].count);

	for(index = offset + row_count; index <= max_index; index++) {
		var row = drow.clone();
		row.addClass('row row-' + index);
		row.attr('row-index', index);
		row.find('input, select, span.form-item').each(function () {
      var fieldName = $(this).attr('field');
			$(this).attr('row-index', index);
			$(this).attr('name', fieldName + '[' + index + ']');
			$(this).attr('id', fieldName + '-' + index);
		});

		// Insert row in table
		row.insertBefore(drow_original);
	}

	// Fill values
	$('#' + gridName + ' tr.row input.unprocessed').each(function () {
		var index = $(this).attr('row-index');
		var fieldName = $(this).attr('name').replace('[' + index + ']', '');
		var id = $(this).attr('id');


    if (typeof fieldName != 'undefined' && typeof Drupal.settings.mywebform.values[fieldName] != 'undefined') {
      $(this).val(Drupal.settings.mywebform.values[fieldName][index - 1]);

      // Add file information
      if (Drupal.settings.mywebform.fields[fieldName].widget == 'file') {
        if (Drupal.settings.mywebform.values[fieldName + '_files'][index - 1]) {
          var file = Drupal.settings.mywebform.values[fieldName + '_files'][index - 1];
          $finfo = $(this).parent().find('.file-info');
          $finfo.find('.file').addClass(file.ext);
          $finfo.find('.filename').html(file.filelink);
          $finfo.find('.filesize').html(file.size);
          $finfo.show();
          $(this).parent().find('#' + fieldName + '-file-' + index).hide();
        }
      }
    }

		// JSCAL
		if($(this).hasClass('date') && id) {
			jscal.manageFields(id, id, "%d.%m.%Y");
		}

		$(this).removeClass('unprocessed');
	});

	$('#' + gridName + ' tr.row select.unprocessed').each(function () {
		var index = $(this).attr('row-index');
		var fieldName = $(this).attr('name').replace('[' + index + ']', '');
		$(this).val(Drupal.settings.mywebform.values[fieldName][index - 1]);
		$(this).removeClass('unprocessed');
	});

	grid_update_pager(gridName);

    validationMarkFields();
}

function grid_remove_rows(gridName, offset) {
	$('#' + gridName + ' tr.row').each(function () {
		var index = parseInt($(this).attr('row-index'));
		if(index < offset || index >= offset + grids[gridName].page_limit) {
			$(this).remove();
		}
	});
}
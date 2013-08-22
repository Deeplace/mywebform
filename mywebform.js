var jscal;
var webform = {
	'afterLoad' : {},
	'validators' : {},
	'errors' : [],
	'warnings' : []
};

Drupal.behaviors.mywebform = function (context) {	
	// Apply field errors got from server
	for(x in Drupal.settings.mywebform.jserrors) {
		webform.errors.push({
			'fieldName' : Drupal.settings.mywebform.jserrors[x].fieldName,
			'index' : parseInt(Drupal.settings.mywebform.jserrors[x].index),
			'msg' : Drupal.settings.mywebform.jserrors[x].msg
		});
	}
	
	$('#mywebform-edit-form input, #mywebform-edit-form select').live('change', function() {
		// Execute synchronization after all validations will be performed
		setTimeout("sync_values($('#" + $(this).attr('id') + "'));", 10);
	});
	
	// Print button
	$('.forprint a').click(function(evt) {
		var href = $(this).attr('href');
		window.open(href, 'new', 'width=800,height=700,toolbar=1,scrollbars=1,resizable=1');
		return false;
	});
	
	// Add jscal to date inputs
	if(!Drupal.settings.mywebform.preview) {
		jscal = Calendar.setup({onSelect: function(jscal) {jscal.hide()}, dateFormat : "%d.%m.%Y"});
		$('input.date').each(function () {
			var id = $(this).attr('id');
			if(id) {
				jscal.manageFields(id, id, "%d.%m.%Y");
			}
		});
	}
	
	$('a.goback').click( function () {
		history.go(-1);
		return false;
	});
	
	// Disable form submit on enter
	function checkForEnter(evt) {
		if (evt.keyCode == 13) {
			var textboxes = $("input:text");
			var currentTextboxNumber = textboxes.index(this);
			if (textboxes[currentTextboxNumber + 1] != null) {
				nextTextbox = textboxes[currentTextboxNumber + 1];
				nextTextbox.select();
			}
			evt.preventDefault();
			return false;
		}
	}
	
	if($.browser.mozilla) {
		$("input:text").live('keypress', checkForEnter);
	} else {
		$("input:text").live('keydown', checkForEnter);
	}
	
	// Fill select boxes with values if they have appended containers
	$('select').live('focus', function () {
		var obj = $(this);
		if(obj.hasClass('filled')) return;
		if(typeof obj.attr('container') == 'undefined') return;
		if(obj.attr('container') == '') return;
		
		//var ta = new Date().getTime();
		obj.addClass('filled');
		
		var elem = document.getElementById(obj.attr('id'));
		var selectedValue = elem.options[elem.selectedIndex].value;
		var i = 0;
		
		obj.empty();		
		try{
			$.each(window[obj.attr('container')], function (key, item) {
				elem.add(new Option(item.value, item.key), null);
				if(item.key == selectedValue) elem.selectedIndex = i;
				i++;
			});
		} catch(e) {
			$.each(window[obj.attr('container')], function (key, item) {
				elem.add(new Option(item.value, item.key));
				if(item.key == selectedValue) elem.selectedIndex = i;
				i++;
			});
		}
		//var tb = new Date().getTime();
		//console.log((tb - ta) + 'ms');
	});
	
	initialize_fiscal_period();
	$('select.period_month, select.period_year, select.period_quarter, select.period_semester, select.period_fond').change(function() {
		set_fiscal_period();
	});
	
	// Show print dialog
	if(Drupal.settings.mywebform.print && !Object.keys(Drupal.settings.mywebform.grids).length) {
		window.print();
	}
	
	// Show upload XML dialog
	if (parseInt(Drupal.settings.mywebform.may_upload_grid)) {
		$('table.grid').each(function () {
			var gridname = $(this).parent().attr('id');
			
			$(this).before('<a class="grid-upload" href="#" grid="' + gridname + '">' +
		Drupal.settings.mywebform.xml_upload_button + '</a>');
		});
		
		$('a.grid-upload').click(function () {
			$('#mywebform-upload-grid-xml-form-wrapper form #edit-grid-name').val($(this).attr('grid'));
			$('#mywebform-upload-grid-xml-form-wrapper').show();
			return false;
		});
		
		$('#mywebform-upload-grid-xml-form-wrapper a.cancel-button').click(function () {
			$('#mywebform-upload-grid-xml-form-wrapper form #edit-grid-name').val($(this).attr('grid'));
			$('#mywebform-upload-grid-xml-form-wrapper').hide();
			return false;
		});
	}
};

function initialize_fiscal_period() {
	var period = $('input.period').val();
	if(period) {
		period = period.split('/');
	} else {
		period = [0, 0, 0];
	}
	if(Drupal.settings.mywebform.preview) {
		switch(period[0]) {
			case 'L':
			case 'l':
				$('span.period_month').text(period[1]);
				$('span.period_year').text(period[2]);
				break;
			case 'T':
			case 't':
				$('span.period_quarter').text(period[1]);
				$('span.period_year').text(period[2]);
				break;
			case 'S':
			case 's':
				$('span.period_semester').text(period[1]);
				$('span.period_year').text(period[2]);
				break;
			case 'A':
			case 'a':
				$('span.period_year').text(period[1]);
				break;
			case 'F':
			case 'f':
				$('span.period_fond').text(period[1]);
				$('span.period_year').text(period[2]);
				break;
		}
	} else {
		switch(period[0]) {
			case 'L':
			case 'l':
				$('select.period_month').val(period[1]);
				$('select.period_year').val(period[2]);
				break;
			case 'T':
			case 't':
				$('select.period_quarter').val(period[1]);
				$('select.period_year').val(period[2]);
				break;
			case 'S':
			case 's':
				$('select.period_semester').val(period[1]);
				$('select.period_year').val(period[2]);
				break;
			case 'A':
			case 'a':
				$('select.period_year').val(period[1]);
				break;
			case 'F':
			case 'f':
				$('select.period_fond').val(period[1]);
				$('select.period_year').val(period[2]);
				break;
		}
	}
}

function set_fiscal_period() {
	var period_type = $('input.period_type').val();
	var month = $('select.period_month').val();
	var quarter = $('select.period_quarter').val();
	var semester = $('select.period_semester').val();
	var fond = $('select.period_fond').val();
	var year = $('select.period_year').val();
	
	var result = '';
	
	switch(period_type) {
		case 'A':
			result = 'A/' + year;
			break;
		case 'L':
			result = 'L/' + month + '/' + year;
			break;
		case 'T':
			result = 'T/' + quarter + '/' + year;
			break;
		case 'S':
			result = 'S/' + semester + '/' + year;
			break;
		case 'F':
			result = 'F/' + fond + '/' + year;
			break;
	}
	$('.period').val(result).trigger('change');
}

if (!Object.keys) {
    Object.keys = function (obj) {
        var keys = [],
            k;
        for (k in obj) {
            if (Object.prototype.hasOwnProperty.call(obj, k)) {
                keys.push(k);
            }
        }
        return keys;
    };
}

/**
 * Synchronizes the value of input or select field with the array of values
 */
function sync_values(elem) {
	var fieldName = elem.attr('field');
	var index = parseInt(elem.attr('row-index'));
	
	if(fieldName) {
		if(index) {
			Drupal.settings.mywebform.values[fieldName][index - 1] = elem.val();
		} else if(!Drupal.settings.mywebform.fields[fieldName].grid_name) {
			if(Drupal.settings.mywebform.fields[fieldName].widget == 'checkbox') {
				Drupal.settings.mywebform.values[fieldName] = elem.is(':checked');
			} else {
				Drupal.settings.mywebform.values[fieldName] = elem.val();
			}
		}

		var autofield_func = Drupal.settings.mywebform.fields[fieldName].autofield;
		if(autofield_func.length) {
			for(x in autofield_func) {
				window[autofield_func[x]](elem);
			}
		}
	}
}

// Special functions to user in autofields

function sum(values) {
	var sum_value = 0.0;
	
	for(x in values) {
		var val = new String(values[x]);
		val = parseFloat(val.replace(',', '.'));
		if(isNaN(val)) val = 0.0;
		sum_value += val;
	}
	
	return sum_value;
}

function row_count(values) {
  var count = 0;
  for(keyvar in values) {
    if(values[keyvar] !== '') {
      count++;
    }
  }

  return count;
}

function min(value1, value2) {
	var v1 = parseFloat(value1);
	var v2 = parseFloat(value2);
	var result = (v1 <= v2) ? v1 : v2;
	return isNaN(result) ? 0 : result;
}

function max(value1, value2) {
	var v1 = parseFloat(value1);
	var v2 = parseFloat(value2);
	var result = (v1 >= v2) ? v1 : v2;
	return isNaN(result) ? 0 : result;
}

function abs(value) {
	return Math.abs(parseFloat(value));
}

function positive_only(value) {
	if(isNaN(value)) return 0;
	return value > 0 ? value : 0;
}

function formatNumber(num, numAfterPoint) {
	var str = num.toString();
	str = str.replace(',', '.');
	if(isNaN(str)) str = new Number(0);
	var formatNum = new Number(str);
	//return formatNum.toFixed(numAfterPoint);	
	var n = Math.pow(10, numAfterPoint);
	return Math.round(formatNum * n + 0.0001) / n;
}

function toFloat(val) {
	if(typeof val === 'boolean') {
		return val ? 1.0 : 0.0;
	}
	try {
		val = new String(val);
		val = parseFloat(val.replace(',', '.'));
		if(isNaN(val)) val = 0.0;
		return val;
	} catch(err) {
		return 0.0;
	}
}

function DEPRECATED_toFloat(obj) {
	try {
		var val = parseFloat(obj.val().replace(',', '.'));
		if(isNaN(val)) val = 0.0;
		return val;
	} catch(err) {
		return 0.0;
	}
}

// Converts a group of checkboxes into a radio set
function radio_checkbox(group, obj) {
	if(obj.attr('field').indexOf(group) == 0) {
		$('input:checkbox').each(function() {
			var fieldName = $(this).attr('field');
			if(fieldName) {
				if(fieldName.indexOf(group) == 0) {
					$(this).attr('checked', false).trigger('change');
				}
			}
		});
		obj.attr('checked', true).trigger('change');
	}
}


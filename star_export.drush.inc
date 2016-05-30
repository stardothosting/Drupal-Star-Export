<?php
/* Work in progress / proof of concept for importing & exporting 
** custom node content + field collections + multi valued field collection data
** Shift8 web : https://www.shift8web.ca
*/

function star_export_drush_command() {
	$currentdate = date('Y_m_d_H_i_s');
	$items['star-export'] = array(
		'description' => 'Export Content to CSV',
		'aliases' => ['star-ex'],
		'callback' => 'star_export',
		'options' => array(
			'f' => 'File to export',
			'c' => 'Field collection name',
			),
	);
        $items['star-import'] = array(
                'description' => 'Import Content from CSV',
                'aliases' => ['star-im'],
                'callback' => 'star_importer',
                'options' => array(
                        'f' => 'File to import',
                        ),
        );

return $items;
}

function star_importer() {

	$file = drush_get_option('f');
	if (!empty($file) && file_exists($file)) {
		$csv_data = array_map('str_getcsv', file($file));
		// loop through array and skip first line
		for ($i = 1; $i < count($csv_data);$i++) {
			$nid = $csv_data[$i][0];
			$title = $csv_data[$i][1];
			$path_alias = $csv_data[$i][2];
			$fc_name = $csv_data[$i][3];
			$fc = $csv_data[$i][4];

			// fail if key fields are not present
			if (empty($nid) || empty($title)) {
				return 'Key fields are missing from array '.$i.'. Check the file and try again';
			}

			// create a new node and assign the data
			$values = array(
				'type' => 'custom_content',
				'uid' => 1,
				'status' => 0,
				);
			$entity = entity_create('node', $values);
			$ewrapper = entity_metadata_wrapper('node', $entity);
			$ewrapper->title->set($title);
			// if there's field collection defined 
			if (!empty($fc)) {
				$fc_items = explode('^', $fc);
				foreach ($fc_items as $fc_item) {
					$fc_array = explode(',', $fc_item);
					$first_opt = $fc_array[0];
					$second_amt = $fc_array[1];
					$fc_nodes = explode('|', $fc_array[2]);
					if (empty($first_opt) || empty($second_amt)) {
						echo 'This assignment is missing either the first or second field option, skipping ..';
						continue;
					}
					$collection = entity_create('field_collection_item', array('field_name' => $fc_name));
					$collection->setHostEntity('node', $entity);
					$cwrapper = entity_metadata_wrapper('field_collection_item', $collection);
					$cwrapper->field_first_opt->set($first_opt);
					$cwrapper->field_second_opt->set($second_opt);
					$cwrapper->field_grp_assigned->set($assigned_nodes);
					$cwrapper->save();  
				}
			} 

			// save
			$ewrapper->save();
		}
	} else {
		return 'No file given to import or file does not exist';
	}

}

function star_export() {

	$fc_name = drush_get_option('c');
	
	// we will use SPL to send our data to STDOUT formatted as CSV
	$fout = new \SplFileObject("php://stdout");
	// write our headers
	$fout->fputcsv([
		'nid', 'title', 'path_alias', 'field_collection_name', 'assigned_nodes'
	]);
	// use a generator to loop through nodes
	foreach (nodes_generator() as $node) {
		$fout->fputcsv([
		// nid
		$node[0]->nid,
		// title
		$node[0]->title,
		// path alias
		url('node/' . $node[0]->nid),
		// field collection name
		$fc_name,	
		// assigned nodes 
		$node[1]
		]);
	}
}

function nodes_generator($field_collection) {
	static $count; // prevent infinite loops
	// query for nodes newer than the specified date
	$query = $query = new EntityFieldQuery();
	$query->entityCondition('entity_type', 'node')
		->propertyCondition('type', 'custom_content')
		->propertyCondition('status', 1);
	$result = $query->execute();
	if (!empty($result)) {
		foreach ($result['node'] as $nid => $row) {
			$count++;
			// TRUE will reset static cache to keep memory usage low
			$node = node_load($row->nid, null, TRUE);
			$collections = array();
			$field_collection_fields = field_get_items('node', $node, 'field_assigned_accessory_dis_grp');
			$collection_item_final = array();
		
			if (!empty($field_collection_fields)) {
				$cnt = 0;
				foreach ($field_collection_fields as $field_collection_field) {
					$item = field_collection_field_get_entity($field_collection_field);
					$collection_item = $item->field_first_option['und'][0]['value'] . ',' .  $item->field_second_option['und'][0]['value'];
					(count($field_collection_fields) >= 2) ? $field_divider = '^' : $field_divider = NULL;
					for ($i = 0;$i < count($item->field_grp_assigned);$i++) {
						$collection_items = array();
						for ($j = 0;$j < count($item->field_grp_assigned['und']);$j++) {
							$collection_items[] = $item->field_grp_assigned['und'][$j]['target_id'];
						}
						// dont add divider if last iteration of loop
						if ($cnt == count($field_collection_fields) - 1) {
							$collection_item_final[] = $collection_item . ',' . implode('|', $collection_items);
						} else { 
							$collection_item_final[] = $collection_item . ',' . implode('|', $collection_items) . $field_divider;
						}		

					}
					$cnt++;
				}
				// return assigned collection 
				yield array($node, implode($collection_item_final));
			}  else {
				// return node without assignments
				yield array($node);
			}
		}
	}
	return;
}
